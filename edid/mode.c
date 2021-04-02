#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

void help();

void main (int argc, char *argv[]) {

    float min_hfreq, max_hfreq;
    float min_pclock, max_pclock;

    uint32_t min_pclock_hz;    
    uint32_t max_pclock_hz;    

    uint32_t min_hfreq_hz;    
    uint32_t max_hfreq_hz;    

    uint32_t hdisplay, htotal;
    uint32_t blanking_percent; /* Percent to sum on hdisplay to get htotal. */

    uint32_t pclock; /* This has to be an integer because we will iterate on it.*/
    float hfreq;
    float vfreq;

    uint32_t vtotal;

    float vfreq_requested, vfreq_real;

    uint32_t i,j;

    if (argv[1] == NULL) {
        help();
    } 
    
    /* REMEMBER that 1st argument is the program name, so it's always passed. */
    if (argc != 6) {
        help;
    }

    hdisplay = atoi(argv[1]);
    vfreq_requested = atof(argv[2]);
    blanking_percent = atoi(argv[3]);
    min_hfreq = atof(argv[4]);
    max_hfreq = atof(argv[5]);

    htotal = hdisplay + (float)hdisplay/100 * blanking_percent;

    /* HFREQ range is the limiting factor, that's why we derive
       PCLOCK from HFREQ and not the opposite.
       HFREQ is in KHz, but PCLOCK is in Hz, that's why we convert
       HFREQ values from KHz to Hz. */
    max_pclock = max_hfreq/1000 * htotal;
    min_pclock = min_hfreq/1000 * htotal;

    printf("hdisplay is %d\n", hdisplay);
    printf("htotal is %d\n", htotal);

    printf("min_pclock is %f\n", min_pclock);
    printf("max_pclock is %f\n", max_pclock);

    min_pclock_hz = min_pclock * 1000000;
    max_pclock_hz = max_pclock * 1000000;

    /* Iterate on all possible pclock values, so we can get their corresponding hfreq,
       vtotal and vfreq.
       It's as if manufacturers gave us a pclock interval instead of an hfreq interval,
       and we get the corresponding hfreqs inside the loop. */

    printf("\n\n***STARTING PCLOCK LOOP***\n\n");

    for (pclock = min_pclock_hz; pclock < max_pclock_hz; pclock++) {

        printf(">>plock %f MHz\n", (float)pclock / 1000000);

        hfreq = (float)pclock / htotal;         
        printf("--hfreq %f KHz\n", hfreq / 1000);

        vtotal = hfreq / vfreq_requested;
        printf("--vtotal %d \n", vtotal);

        vfreq_real = hfreq / vtotal;
        printf("--freq_real %f \n", vfreq_real);

        printf("\n");
    }
   
}


void help () {
       printf("NOTE: min_hfreq and max_hfreq are values you must get from your monitor EDID.\n");
       printf("Syntax: <hdisplay> <vfreq_requested> <blanking_percent> <min_hfreq (KHz)> <max_hfreq (KHz)>\n");
       exit (0);
}
