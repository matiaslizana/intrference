/*==============================================================================
Custom DSP Example
Copyright (c), Firelight Technologies Pty, Ltd 2004-2018.

This example shows how to add a user created DSP callback to process audio 
data. The read callback is executed at runtime, and can be added anywhere in
the DSP network.
==============================================================================*/
#include "fmod.hpp"
#include "common.h"

typedef struct 
{
    float *buffer;
	
	//Params
	float voice_shatter;
	float noise_volume;
	float noise_shatter;

	//Struct params
	int   length_samples;
    int   channels;
} mydsp_data_t;

FMOD_RESULT F_CALLBACK myDSPCallback(FMOD_DSP_STATE *dsp_state, float *inbuffer, float *outbuffer, unsigned int length, int inchannels, int *outchannels) 
{

	mydsp_data_t *data = (mydsp_data_t *)dsp_state->plugindata;

	//Volume that voice will have on each "window"
	float voice_shatter = 1 - (((float)(rand() % 32768) / 16384.0f) - 1.0f) * data->voice_shatter;
	float noise_shatter = 1 - (((float)(rand() % 32768) / 16384.0f) - 1.0f) * data->noise_shatter;

	for (unsigned int samp = 0; samp < length; samp++) 
    { 
        for (int chan = 0; chan < *outchannels; chan++)
        {
			float noise = (((float)(rand() % 32768) / 16384.0f) - 1.0f) * data->noise_volume * 0.1f * noise_shatter;
			outbuffer[(samp * inchannels) + chan] = inbuffer[(samp * inchannels) + chan] * voice_shatter + noise;
			data->buffer[(samp * *outchannels) + chan] = outbuffer[(samp * inchannels) + chan];
        }
    }

    //data->channels = inchannels;

    return FMOD_OK; 
} 

/*
    Callback called when DSP is created.   This implementation creates a structure which is attached to the dsp state's 'plugindata' member.
*/
FMOD_RESULT F_CALLBACK myDSPCreateCallback(FMOD_DSP_STATE *dsp_state)
{
    unsigned int blocksize;
    FMOD_RESULT result;

    result = dsp_state->functions->getblocksize(dsp_state, &blocksize);
    ERRCHECK(result);

    mydsp_data_t *data = (mydsp_data_t *)calloc(sizeof(mydsp_data_t), 1);
    if (!data)
    {
        return FMOD_ERR_MEMORY;
    }
    dsp_state->plugindata = data;
	data->voice_shatter = 0.0f;
	data->noise_volume = 0.0f;
	data->noise_shatter = 0.0f;
    data->length_samples = blocksize;

    data->buffer = (float *)malloc(blocksize * 8 * sizeof(float));      // *8 = maximum size allowing room for 7.1.   Could ask dsp_state->functions->getspeakermode for the right speakermode to get real speaker count.
    if (!data->buffer)
    {
        return FMOD_ERR_MEMORY;
    }

    return FMOD_OK;
}

/*
    Callback called when DSP is destroyed.   The memory allocated in the create callback can be freed here.
*/
FMOD_RESULT F_CALLBACK myDSPReleaseCallback(FMOD_DSP_STATE *dsp_state)
{
    if (dsp_state->plugindata)
    {
        mydsp_data_t *data = (mydsp_data_t *)dsp_state->plugindata;

        if (data->buffer)
        {
            free(data->buffer);
        }

        free(data);
    }

    return FMOD_OK;
}

/*
    Callback called when DSP::getParameterData is called.   This returns a pointer to the raw floating point PCM data.
    We have set up 'parameter 0' to be the data parameter, so it checks to make sure the passed in index is 0, and nothing else.
*/
FMOD_RESULT F_CALLBACK myDSPGetParameterDataCallback(FMOD_DSP_STATE *dsp_state, int index, void **data, unsigned int *length, char *)
{
    if (index == 0)
    {
        unsigned int blocksize;
        FMOD_RESULT result;
        mydsp_data_t *mydata = (mydsp_data_t *)dsp_state->plugindata;

        result = dsp_state->functions->getblocksize(dsp_state, &blocksize);
        ERRCHECK(result);

        *data = (void *)mydata;
        *length = blocksize * 2 * sizeof(float);

        return FMOD_OK;
    }

    return FMOD_ERR_INVALID_PARAM;
}

/*
    Callback called when DSP::setParameterFloat is called.   This accepts a floating point 0 to 1 volume value, and stores it.
    We have set up 'parameter 1' to be the volume parameter, so it checks to make sure the passed in index is 1, and nothing else.
*/
FMOD_RESULT F_CALLBACK myDSPSetParameterFloatCallback(FMOD_DSP_STATE *dsp_state, int index, float value)
{
	mydsp_data_t *mydata = (mydsp_data_t *)dsp_state->plugindata;
	if (index == 1)
		mydata->voice_shatter = value;
	else if (index == 2)
		mydata->noise_volume = value;
	else if (index == 3)
		mydata->noise_shatter = value;
	else
		return FMOD_ERR_INVALID_PARAM;
	return FMOD_OK;
}

/*
    Callback called when DSP::getParameterFloat is called.   This returns a floating point 0 to 1 volume value.
    We have set up 'parameter 1' to be the volume parameter, so it checks to make sure the passed in index is 1, and nothing else.
    An alternate way of displaying the data is provided, as a string, so the main app can use it.
*/
FMOD_RESULT F_CALLBACK myDSPGetParameterFloatCallback(FMOD_DSP_STATE *dsp_state, int index, float *value, char *valstr)
{
	mydsp_data_t *mydata = (mydsp_data_t *)dsp_state->plugindata;

	if (index == 1)
		*value = mydata->voice_shatter;
	else if (index == 2)
		*value = mydata->noise_volume;
	else if (index == 3)
		*value = mydata->noise_shatter;
	else
		return FMOD_ERR_INVALID_PARAM;

	if (valstr)
		sprintf(valstr, "%d", (int)((*value * 100.0f) + 0.5f));
	
	return FMOD_OK;
}

int FMOD_Main()
{
    FMOD::System       *system;
    FMOD::Sound        *sound;
    FMOD::Channel      *channel;
    FMOD::DSP          *mydsp;
    FMOD::ChannelGroup *mastergroup;
    FMOD_RESULT         result;
    unsigned int        version;
    void               *extradriverdata = 0;

    Common_Init(&extradriverdata);

    /*
        Create a System object and initialize.
    */
    result = FMOD::System_Create(&system);
    ERRCHECK(result);

    result = system->getVersion(&version);
    ERRCHECK(result);

    if (version < FMOD_VERSION)
    {
        Common_Fatal("FMOD lib version %08x doesn't match header version %08x", version, FMOD_VERSION);
    }

    result = system->init(32, FMOD_INIT_NORMAL, extradriverdata);
    ERRCHECK(result);

    result = system->createSound(Common_MediaPath("talk.ogg"), FMOD_LOOP_NORMAL, 0, &sound);
    ERRCHECK(result);

    result = system->playSound(sound, 0, false, &channel);
    ERRCHECK(result);

    /*
        Create the DSP effect.
    */  
    { 
        FMOD_DSP_DESCRIPTION dspdesc; 
        memset(&dspdesc, 0, sizeof(dspdesc));
        FMOD_DSP_PARAMETER_DESC wavedata_desc;
        FMOD_DSP_PARAMETER_DESC voice_shatter_desc;
		FMOD_DSP_PARAMETER_DESC noise_volume_desc;
		FMOD_DSP_PARAMETER_DESC noise_shatter_desc;
        FMOD_DSP_PARAMETER_DESC *paramdesc[4] = 
        {
            &wavedata_desc,
            &voice_shatter_desc,
			&noise_volume_desc,
			&noise_shatter_desc
        };

        FMOD_DSP_INIT_PARAMDESC_DATA(wavedata_desc, "wave data", "", "wave data", FMOD_DSP_PARAMETER_DATA_TYPE_USER);
        FMOD_DSP_INIT_PARAMDESC_FLOAT(voice_shatter_desc, "voice shatter", "%", "voice shatter in percent", 0, 1, 1);
		FMOD_DSP_INIT_PARAMDESC_FLOAT(noise_volume_desc, "noise volume", "%", "noise volume in percent", 0, 1, 1);
		FMOD_DSP_INIT_PARAMDESC_FLOAT(noise_shatter_desc, "noise shatter", "%", "noise shatter in percent", 0, 1, 1);

        strncpy(dspdesc.name, "My first DSP unit", sizeof(dspdesc.name));
        dspdesc.version             = 0x00010000;
        dspdesc.numinputbuffers     = 1;
        dspdesc.numoutputbuffers    = 1;
        dspdesc.read                = myDSPCallback; 
        dspdesc.create              = myDSPCreateCallback;
        dspdesc.release             = myDSPReleaseCallback;
        dspdesc.getparameterdata    = myDSPGetParameterDataCallback;
        dspdesc.setparameterfloat   = myDSPSetParameterFloatCallback;
        dspdesc.getparameterfloat   = myDSPGetParameterFloatCallback;
        dspdesc.numparameters       = 4;
        dspdesc.paramdesc           = paramdesc;

        result = system->createDSP(&dspdesc, &mydsp); 
        ERRCHECK(result); 
    } 

    result = system->getMasterChannelGroup(&mastergroup);
    ERRCHECK(result);

    result = mastergroup->addDSP(0, mydsp);
    ERRCHECK(result);

    /*
        Main loop.
    */
    do
    {
        bool bypass;

        Common_Update();

        result = mydsp->getBypass(&bypass);
        ERRCHECK(result);

        if (Common_BtnPress(BTN_ACTION1))
        {
            bypass = !bypass;
            
            result = mydsp->setBypass(bypass);
            ERRCHECK(result);
        }        
        if (Common_BtnPress(BTN_ACTION2))
        {
            float voice_shatter;

            result = mydsp->getParameterFloat(1, &voice_shatter, 0, 0);
            ERRCHECK(result);

            if (voice_shatter > 0.0f)
				voice_shatter -= 0.1f;

            result = mydsp->setParameterFloat(1, voice_shatter);
            ERRCHECK(result);
        }
        if (Common_BtnPress(BTN_ACTION3))
        {
            float voice_shatter;

            result = mydsp->getParameterFloat(1, &voice_shatter, 0, 0);
            ERRCHECK(result);

            if (voice_shatter < 1.0f)
				voice_shatter += 0.1f;

            result = mydsp->setParameterFloat(1, voice_shatter);
            ERRCHECK(result);
        }

		if (Common_BtnPress(BTN_DOWN))
		{
			float noise_volume;

			result = mydsp->getParameterFloat(2, &noise_volume, 0, 0);
			ERRCHECK(result);

			if (noise_volume > 0.0f)
				noise_volume -= 0.1f;

			result = mydsp->setParameterFloat(2, noise_volume);
			ERRCHECK(result);
		}
		if (Common_BtnPress(BTN_UP))
		{
			float noise_volume;

			result = mydsp->getParameterFloat(2, &noise_volume, 0, 0);
			ERRCHECK(result);

			if (noise_volume < 1.0f)
				noise_volume += 0.1f;

			result = mydsp->setParameterFloat(2, noise_volume);
			ERRCHECK(result);
		}

		if (Common_BtnPress(BTN_LEFT))
		{
			float noise_shatter;

			result = mydsp->getParameterFloat(3, &noise_shatter, 0, 0);
			ERRCHECK(result);

			if (noise_shatter > 0.0f)
				noise_shatter -= 0.1f;

			result = mydsp->setParameterFloat(3, noise_shatter);
			ERRCHECK(result);
		}
		if (Common_BtnPress(BTN_RIGHT))
		{
			float noise_shatter;

			result = mydsp->getParameterFloat(3, &noise_shatter, 0, 0);
			ERRCHECK(result);

			if (noise_shatter < 1.0f)
				noise_shatter += 0.1f;

			result = mydsp->setParameterFloat(3, noise_shatter);
			ERRCHECK(result);
		}

        result = system->update();
        ERRCHECK(result);

        {
            char                     voice_shatter_str[32] = { 0 };
			char					 noise_volume_str[32] = { 0 };
			char					 noise_shatter_str[32] = { 0 };
            FMOD_DSP_PARAMETER_DESC *desc;
            mydsp_data_t            *data;

            result = mydsp->getParameterInfo(1, &desc);
            ERRCHECK(result);
            result = mydsp->getParameterFloat(1, 0, voice_shatter_str, 32);
            ERRCHECK(result);
			result = mydsp->getParameterFloat(2, 0, noise_volume_str, 32);
			ERRCHECK(result);
			result = mydsp->getParameterFloat(3, 0, noise_shatter_str, 32);
			ERRCHECK(result);
			result = mydsp->getParameterData(0, (void **)&data, 0, 0, 0);
            ERRCHECK(result);

            Common_Draw("==================================================");
            Common_Draw("Interference Plugin v0.1 for FMOD");
            Common_Draw("Matias Lizana");
            Common_Draw("==================================================");
            Common_Draw("");
            Common_Draw("Press %s to toggle filter bypass", Common_BtnStr(BTN_ACTION1));

            Common_Draw("Press %s to decrease voice shatter 10%%", Common_BtnStr(BTN_ACTION2));
            Common_Draw("Press %s to increase voice shatter 10%%", Common_BtnStr(BTN_ACTION3));
			Common_Draw("Press %s to decrease noise volume 10%%", Common_BtnStr(BTN_DOWN));
			Common_Draw("Press %s to increase noise volume 10%%", Common_BtnStr(BTN_UP));
			Common_Draw("Press %s to decrease noise shatter 10%%", Common_BtnStr(BTN_LEFT));
			Common_Draw("Press %s to increase noise shatter 10%%", Common_BtnStr(BTN_RIGHT));
			Common_Draw("Press %s to quit", Common_BtnStr(BTN_QUIT));
            Common_Draw("");
            Common_Draw("Filter is %s", bypass ? "inactive" : "active");
			Common_Draw("Voice Shatter is %s", voice_shatter_str);
			Common_Draw("Noise Volume is %s", noise_volume_str);
			Common_Draw("Noise Shatter is %s", noise_shatter_str);


            if (data->channels)
            {
                char display[80] = { 0 };
                int channel;

                for (channel = 0; channel < data->channels; channel++)
                {
                    int count,level;
                    float max = 0;

                    for (count = 0; count < data->length_samples; count++)
                    {
                        if (fabs(data->buffer[(count * data->channels) + channel]) > max)
                        {
                            max = fabs(data->buffer[(count * data->channels) + channel]);
                        }
                    }
                    level = max * 40.0f;
                    
                    sprintf(display, "%2d ", channel);
                    for (count = 0; count < level; count++) display[count + 3] = '=';

                    Common_Draw(display);
                }
            }
        }

        Common_Sleep(50);
    } while (!Common_BtnPress(BTN_QUIT));

    /*
        Shut down
    */
    result = sound->release();
    ERRCHECK(result);

    result = mastergroup->removeDSP(mydsp);
    ERRCHECK(result);
    result = mydsp->release();
    ERRCHECK(result);

    result = system->close();
    ERRCHECK(result);
    result = system->release();
    ERRCHECK(result);

    Common_Close();

    return 0;
}
