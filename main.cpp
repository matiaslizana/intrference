/*==============================================================================
Custom DSP Example
Copyright (c), Firelight Technologies Pty, Ltd 2004-2018.

This example shows how to add a user created DSP callback to process audio 
data. The read callback is executed at runtime, and can be added anywhere in
the DSP network.
==============================================================================*/
#include "fmod.hpp"
#include "common.h"

int FMOD_Main()
{
    FMOD::System       *system;
    FMOD::Sound        *sound;
    FMOD::Channel      *channel;
    FMOD::DSP          *interf_dsp;
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

	{
		FMOD_DSP_DESCRIPTION dspdesc;
		memset(&dspdesc, 0, sizeof(dspdesc));
		FMOD_DSP_PARAMETER_DESC voice_shatter_desc;
		FMOD_DSP_PARAMETER_DESC noise_volume_desc;
		FMOD_DSP_PARAMETER_DESC noise_shatter_desc;
		FMOD_DSP_PARAMETER_DESC *paramdesc[3] =
		{
			//&wavedata_desc,
			&voice_shatter_desc,
			&noise_volume_desc,
			&noise_shatter_desc
		};

		FMOD_DSP_INIT_PARAMDESC_FLOAT(voice_shatter_desc, "Voice Shatter", "%", "voice shatter in percent", 0, 1, 1);
		FMOD_DSP_INIT_PARAMDESC_FLOAT(noise_volume_desc, "Noise Volume", "%", "noise volume in percent", 0, 1, 1);
		FMOD_DSP_INIT_PARAMDESC_FLOAT(noise_shatter_desc, "Noise Shatter", "%", "noise shatter in percent", 0, 1, 1);

		strncpy(dspdesc.name, "My first DSP unit", sizeof(dspdesc.name));
		dspdesc.version = 0x00010000;
		dspdesc.numinputbuffers = 1;
		dspdesc.numoutputbuffers = 1;
		dspdesc.numparameters = 3;
		dspdesc.paramdesc = paramdesc;

		result = system->createDSP(&dspdesc, &interf_dsp);
		ERRCHECK(result);
	}

    result = system->getMasterChannelGroup(&mastergroup);
    ERRCHECK(result);

    result = mastergroup->addDSP(0, interf_dsp);
    ERRCHECK(result);

    /*
        Main loop.
    */
    do
    {
        bool bypass;

        Common_Update();

        result = interf_dsp->getBypass(&bypass);
        ERRCHECK(result);

        if (Common_BtnPress(BTN_ACTION1))
        {
            bypass = !bypass;
            
            result = interf_dsp->setBypass(bypass);
            ERRCHECK(result);
        }        
        if (Common_BtnPress(BTN_ACTION2))
        {
            float voice_shatter;

            result = interf_dsp->getParameterFloat(1, &voice_shatter, 0, 0);
            ERRCHECK(result);

            if (voice_shatter > 0.0f)
				voice_shatter -= 0.1f;

            result = interf_dsp->setParameterFloat(1, voice_shatter);
            ERRCHECK(result);
        }
        if (Common_BtnPress(BTN_ACTION3))
        {
            float voice_shatter;

            result = interf_dsp->getParameterFloat(1, &voice_shatter, 0, 0);
            ERRCHECK(result);

            if (voice_shatter < 1.0f)
				voice_shatter += 0.1f;

            result = interf_dsp->setParameterFloat(1, voice_shatter);
            ERRCHECK(result);
        }

		if (Common_BtnPress(BTN_DOWN))
		{
			float noise_volume;

			result = interf_dsp->getParameterFloat(2, &noise_volume, 0, 0);
			ERRCHECK(result);

			if (noise_volume > 0.0f)
				noise_volume -= 0.1f;

			result = interf_dsp->setParameterFloat(2, noise_volume);
			ERRCHECK(result);
		}
		if (Common_BtnPress(BTN_UP))
		{
			float noise_volume;

			result = interf_dsp->getParameterFloat(2, &noise_volume, 0, 0);
			ERRCHECK(result);

			if (noise_volume < 1.0f)
				noise_volume += 0.1f;

			result = interf_dsp->setParameterFloat(2, noise_volume);
			ERRCHECK(result);
		}

		if (Common_BtnPress(BTN_LEFT))
		{
			float noise_shatter;

			result = interf_dsp->getParameterFloat(3, &noise_shatter, 0, 0);
			ERRCHECK(result);

			if (noise_shatter > 0.0f)
				noise_shatter -= 0.1f;

			result = interf_dsp->setParameterFloat(3, noise_shatter);
			ERRCHECK(result);
		}
		if (Common_BtnPress(BTN_RIGHT))
		{
			float noise_shatter;

			result = interf_dsp->getParameterFloat(3, &noise_shatter, 0, 0);
			ERRCHECK(result);

			if (noise_shatter < 1.0f)
				noise_shatter += 0.1f;

			result = interf_dsp->setParameterFloat(3, noise_shatter);
			ERRCHECK(result);
		}

        result = system->update();
        ERRCHECK(result);

        {
            char                     voice_shatter_str[32] = { 0 };
			char					 noise_volume_str[32] = { 0 };
			char					 noise_shatter_str[32] = { 0 };
            FMOD_DSP_PARAMETER_DESC *desc;
            //mydsp_data_t            *data;

            result = interf_dsp->getParameterInfo(1, &desc);
            ERRCHECK(result);
            result = interf_dsp->getParameterFloat(1, 0, voice_shatter_str, 32);
            ERRCHECK(result);
			result = interf_dsp->getParameterFloat(2, 0, noise_volume_str, 32);
			ERRCHECK(result);
			result = interf_dsp->getParameterFloat(3, 0, noise_shatter_str, 32);
			ERRCHECK(result);
			//result = interf_dsp->getParameterData(0, (void **)&data, 0, 0, 0);
            //ERRCHECK(result);

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

			/*
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
			*/
        }

        Common_Sleep(50);
    } while (!Common_BtnPress(BTN_QUIT));

    /*
        Shut down
    */
    result = sound->release();
    ERRCHECK(result);

    result = mastergroup->removeDSP(interf_dsp);
    ERRCHECK(result);
    result = interf_dsp->release();
    ERRCHECK(result);

    result = system->close();
    ERRCHECK(result);
    result = system->release();
    ERRCHECK(result);

    Common_Close();

    return 0;
}
