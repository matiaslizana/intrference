/*==========================================
IntRference Plugin v0.1 for FMOD
Matias Lizana Garc�a 2018
===========================================*/

#define _CRT_SECURE_NO_WARNINGS

#include "fmod.hpp"
#include "common.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

extern "C" {
	F_EXPORT FMOD_DSP_DESCRIPTION* F_CALL FMODGetDSPDescription();
}

#define INTRF_NUM_PARAMETERS 3

FMOD_RESULT F_CALLBACK IntrfReadCallback(FMOD_DSP_STATE *dsp_state, float *inbuffer, float *outbuffer, unsigned int length, int inchannels, int *outchannels);
FMOD_RESULT F_CALLBACK IntrfCreateCallback(FMOD_DSP_STATE *dsp_state);
FMOD_RESULT F_CALLBACK IntrfReleaseCallback(FMOD_DSP_STATE *dsp_state);
FMOD_RESULT F_CALLBACK IntrfResetCallback(FMOD_DSP_STATE *dsp_state);
FMOD_RESULT F_CALLBACK IntrfSetParamFloatCallback(FMOD_DSP_STATE *dsp_state, int index, float value);
FMOD_RESULT F_CALLBACK IntrfGetParamFloatCallback(FMOD_DSP_STATE *dsp_state, int index, float *value, char *valstr);

static FMOD_DSP_PARAMETER_DESC voice_shatter_desc;
static FMOD_DSP_PARAMETER_DESC noise_volume_desc;
static FMOD_DSP_PARAMETER_DESC noise_shatter_desc;

FMOD_DSP_PARAMETER_DESC *paramdesc[INTRF_NUM_PARAMETERS] =
{
	&voice_shatter_desc,
	&noise_volume_desc,
	&noise_shatter_desc
};

FMOD_DSP_DESCRIPTION FMOD_Intrference_Desc =
{
	FMOD_PLUGIN_SDK_VERSION,
	"intRference",  // name
	0x00010000,     // plug-in version
	1,              // number of input buffers to process
	1,              // number of output buffers to process
	IntrfCreateCallback,
	IntrfReleaseCallback,
	IntrfResetCallback,
	IntrfReadCallback,
	0,
	0,
	INTRF_NUM_PARAMETERS,
	paramdesc,
	IntrfSetParamFloatCallback,
	0,
	0,
	0,
	IntrfGetParamFloatCallback,
	0,
	0,
	0, 
	0, 
	0,   
	0,
	0,
	0
};

extern "C"
{

	F_EXPORT FMOD_DSP_DESCRIPTION* F_CALL FMODGetDSPDescription()
	{
		FMOD_DSP_INIT_PARAMDESC_FLOAT(voice_shatter_desc, "Voice Shatter", "%", "voice shatter in percent", 0, 100, 0);
		FMOD_DSP_INIT_PARAMDESC_FLOAT(noise_volume_desc, "Noise Volume", "%", "noise volume in percent", 0, 100, 0);
		FMOD_DSP_INIT_PARAMDESC_FLOAT(noise_shatter_desc, "Noise Shatter", "%", "noise shatter in percent", 0, 100, 0);
		return &FMOD_Intrference_Desc;
	}

}

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
} intrf_data;

FMOD_RESULT F_CALLBACK IntrfReadCallback(FMOD_DSP_STATE *dsp_state, float *inbuffer, float *outbuffer, unsigned int length, int inchannels, int *outchannels) 
{

	intrf_data *data = (intrf_data *)dsp_state->plugindata;

	//Volume that voice will have on each "window"
	float data_vs = (float)(data->voice_shatter / 100);
	float data_ns = (float)(data->noise_shatter / 100);
	float data_nv = (float)(data->noise_volume / 100);
	float voice_shatter = 1 - (((float)(rand() % 32768) / 16384.0f) - 1.0f) * data_vs;
	float noise_shatter = 1 - (((float)(rand() % 32768) / 16384.0f) - 1.0f) * data_ns;

	for (unsigned int samp = 0; samp < length; samp++) 
    { 
        for (int chan = 0; chan < *outchannels; chan++)
        {
			float noise = (((float)(rand() % 32768) / 16384.0f) - 1.0f) * data_nv * 0.1f * noise_shatter;
			outbuffer[(samp * inchannels) + chan] = inbuffer[(samp * inchannels) + chan] * voice_shatter + noise;
			data->buffer[(samp * *outchannels) + chan] = outbuffer[(samp * inchannels) + chan];
        }
    }

    return FMOD_OK; 
} 

FMOD_RESULT F_CALLBACK IntrfCreateCallback(FMOD_DSP_STATE *dsp_state)
{
    unsigned int blocksize;
    FMOD_RESULT result;

    result = dsp_state->functions->getblocksize(dsp_state, &blocksize);
    //ERRCHECK(result);

    intrf_data *data = (intrf_data *)calloc(sizeof(intrf_data), 1);
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

FMOD_RESULT F_CALLBACK IntrfReleaseCallback(FMOD_DSP_STATE *dsp_state)
{
    if (dsp_state->plugindata)
    {
        intrf_data *data = (intrf_data *)dsp_state->plugindata;

        if (data->buffer)
        {
			free(data->buffer);
        }
        free(data);
    }

    return FMOD_OK;
}

FMOD_RESULT F_CALLBACK IntrfResetCallback(FMOD_DSP_STATE *dsp_state) {
	intrf_data *data = (intrf_data *)dsp_state->plugindata;
	data->voice_shatter = 0.0f;
	data->noise_volume = 0.0f;
	data->noise_shatter = 0.0f;
	return FMOD_OK;
}

FMOD_RESULT F_CALLBACK IntrfSetParamFloatCallback(FMOD_DSP_STATE *dsp_state, int index, float value)
{
	intrf_data *mydata = (intrf_data *)dsp_state->plugindata;
	if (index == 0)
		mydata->voice_shatter = value;
	else if (index == 1)
		mydata->noise_volume = value;
	else if (index == 2)
		mydata->noise_shatter = value;
	else
		return FMOD_ERR_INVALID_PARAM;
	return FMOD_OK;
}

FMOD_RESULT F_CALLBACK IntrfGetParamFloatCallback(FMOD_DSP_STATE *dsp_state, int index, float *value, char *)
{
	intrf_data *mydata = (intrf_data *)dsp_state->plugindata;

	if (index == 0)
		*value = mydata->voice_shatter;
	else if (index == 1)
		*value = mydata->noise_volume;
	else if (index == 2)
		*value = mydata->noise_shatter;
	else
		return FMOD_ERR_INVALID_PARAM;
	
	return FMOD_OK;
}