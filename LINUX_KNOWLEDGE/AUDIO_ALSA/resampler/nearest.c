#include <stdio.h>
#include <stdint.h>

// The simulated frames the resampler is going to run for.
// No need for timming control: averaging works without it.
// If we run for 60 frames and we suppose we run at 60.0000 fps,
// the we should get the exact number of samples per second we
// specify for the output_frequency in the accum_output_samples variable
// after we run for 60 frames.
#define FRAMES_TO_RUN 60

struct resampler_t {
	float output_freq;
	float input_freq;
	float ratio;
	float fraction;
	float threshold;
	//Number of samples produced on each run on the resampler process function.
	float output_samples; 
	// Number of input samples. It's constant among video frames. But if
	// it's not, theres no problem: the compensation mechanism 
	// towards the desired outpur freq works the same!
	float input_samples; 
};

struct resampler_t resampler;

void process () {
	int nsamples = 0;
	while (nsamples < resampler.input_samples  ) { 
		while (resampler.fraction > resampler.threshold) {
			resampler.output_samples++;	
			resampler.fraction -= resampler.ratio;
		} 	
		resampler.fraction += resampler.threshold;
		nsamples++;
	}
	return;
}

int main () {
	int frames;
	resampler.output_freq = 44100.0;
	resampler.input_freq =  32000.0;
	resampler.threshold = 1.0;
	resampler.fraction = 0;
	resampler.output_samples = 0;
	float ratio = resampler.output_freq / resampler.input_freq;	
	// In this test implementation, we leave the number of samples as a constant
	// for now. But it can change between resampler calls (ie, between video frames)
	// and things should still work.
	// We are chosing 533.333333 samples per frame because we imagine, for the 
	// PRODUCTOR side (the INPUT side, game or emu) we IMAGINE that we are 
	// doing 60.0000 fps at 32000 samples per second, so 
	// 320000 / 60.0000 = 533.333333 samples per frame.
	// For 44100 sps and 60.016804 fps, we sould have 735 samples per frame.
	resampler.input_samples = 533.333333;
	// We divide the threshold value in a number of parts. That number of parts
	// equals the output / input ratio. Then we will sum an integer number of
	// these parts to evaluate when we should exit the copy loop and change the
	// input element. 
	resampler.ratio = resampler.threshold / ratio; 

	// The total frames produced across all frames we run for.
	float accum_output_samples = 0;

	for (frames = 0; frames < FRAMES_TO_RUN; frames++) {
		// We need to set fraction to the threshold before we enter process
		// for the first time since we will increase the input from
		// the first time we enter the outer loop, so we need to enter the inner
		// loop (the copy loop) from the first time.
		// Also we need to reset the fraction each frame so we don't
		// carry on the remainings to the next, since that causes
		// too many iterations on first copy loop enter in each frame.
		resampler.fraction = resampler.threshold;
		
		process();	
		printf ("ratio %f\n", resampler.ratio);
		printf ("input samples this video frame %f\n",
			 resampler.input_samples);
		printf("output samples this video frame %f\n\n",
			resampler.output_samples);
		accum_output_samples += resampler.output_samples;
		
		resampler.output_samples = 0;
	}
	printf ("Total output samples in %d frames: %f\n", FRAMES_TO_RUN, accum_output_samples);
	return 0;
}
