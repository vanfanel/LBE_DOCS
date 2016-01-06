#include <stdio.h>
#include <stdbool.h>
#include <alsa/asoundlib.h>

//#define SOUND_SAMPLERATE 22255 /* = round(7833600 * 2 / 704) */
#define SOUND_SAMPLERATE 44100

/****************************************************************************************************
 * This small program has two blocks that can be commented or used alternatively:
 * - A block that uses snd_pcm_set_params() to set most parameters in a single call.
 *   This function will decide period size for us internally!! So the buffersize/periodsize 
 *   proportion can vary between depending on what delay we specify (see how it works internally
 *   if you're interested, but meh...). 
 * - A block that sets the parameters in a more "manual" way by using several snd_pcm_hw_params_set*
 *   funtion calls. We specify buffersize as a periodsize * num_periods product in the right function.
 *
 * On the first block, using snd_pcm_set_params() you chose the delay directly by passing it in microseconds
 * (1 second = 10^6 microseconds)
 * On the second block, you chose the periodsize and number of periods, and delay is a consequence.
 *****************************************************************************************************/

int main ()
{
	snd_pcm_t *pcm_handle;
	snd_pcm_hw_params_t *hwparams = NULL;
	snd_pcm_uframes_t buffer_size_frames;
	snd_pcm_uframes_t period_size_frames;
	size_t buffer_size_bytes;
        size_t period_size_bytes;

	const char *alsa_dev = strdup("plughw:0,0");	
	//const char *alsa_dev = strdup("hw:0,0");	

	snd_pcm_open(&pcm_handle, alsa_dev, SND_PCM_STREAM_PLAYBACK, 0);
	
	snd_pcm_hw_params_malloc(&hwparams);

	// Delay parameter in this function is related to period, not buffersize.	
	/*int ret = snd_pcm_set_params(pcm_handle,
                           SND_PCM_FORMAT_S16_LE,
                           SND_PCM_ACCESS_RW_INTERLEAVED,
                           2,
                           SOUND_SAMPLERATE,
                           0,
			   2902);	
	
	printf ("ret %d\n", ret);*/
	

	// Manual parameters setting block
	/* Init hwparams with full configuration space */
	
	int periodsize = 512;
	int periods = 2;
	if (snd_pcm_hw_params_any(pcm_handle, hwparams) < 0) {
         	fprintf(stderr, "Can not configure this PCM device.\n");
		return(-1);
	}

	if (snd_pcm_hw_params_set_access(pcm_handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED) < 0) {
      		fprintf(stderr, "Error setting access.\n");
      		return(-1);
    	}	

	// Set sample format
   	if (snd_pcm_hw_params_set_format(pcm_handle, hwparams, SND_PCM_FORMAT_S16_LE) < 0) {
      		fprintf(stderr, "Error setting format.\n");
      		return(-1);
    	}
	// Set sample rate. If the exact rate is not supported
	// by the hardware, use nearest possible rate. 
	int exact_rate = SOUND_SAMPLERATE;
	if (snd_pcm_hw_params_set_rate_near(pcm_handle, hwparams, &exact_rate, 0) < 0) {
		fprintf(stderr, "Error setting rate.\n");
		return(-1);
	}
	if (SOUND_SAMPLERATE != exact_rate) {
		fprintf(stderr, "The rate %d Hz is not supported by your hardware. Using %d Hz instead.\n",
			SOUND_SAMPLERATE, exact_rate);
	}

	// Set number of channels
	if (snd_pcm_hw_params_set_channels(pcm_handle, hwparams, 2) < 0) {
		fprintf(stderr, "Error setting channels.\n");
		return(-1);
	}

	// Set number of periods. Periods used to be called fragments.
	if (snd_pcm_hw_params_set_periods(pcm_handle, hwparams, periods , 0) < 0) {
		fprintf(stderr, "Error setting periods.\n");
		return(-1);
	}

	// Set buffer size (in frames). The resulting latency is given by
        // latency = periodsize * periods / (rate * bytes_per_frame)
	if (snd_pcm_hw_params_set_buffer_size(pcm_handle, hwparams, (periodsize * periods)>>2) < 0) {
		fprintf(stderr, "Error setting buffersize.\n");
		return(-1);
	}

	// Apply HW parameter settings to
	// PCM device and prepare device
	if (snd_pcm_hw_params(pcm_handle, hwparams) < 0) {
		fprintf(stderr, "Error setting HW params.\n");
		return(-1);
	}
	// Manual parameters setting block ENDS	

	snd_pcm_get_params (pcm_handle, &buffer_size_frames, &period_size_frames);

	buffer_size_bytes = snd_pcm_frames_to_bytes(pcm_handle, buffer_size_frames);
        period_size_bytes = snd_pcm_frames_to_bytes(pcm_handle, period_size_frames);

	printf("ALSA: Period size: %d frames\n", (int)period_size_frames);
        printf("ALSA: Buffer size: %d frames\n", (int)buffer_size_frames);
       	printf("ALSA: Period size: %d bytes\n", (int)period_size_bytes);
        printf("ALSA: Buffer size: %d bytes\n", (int)buffer_size_bytes);
}
