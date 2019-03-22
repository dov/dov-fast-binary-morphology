// Test the morphing library
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "bimg.h"
#include "morph.h"
#include <sys/time.h>
#include <stdio.h>


static void die(const char *fmt, ...)
{
    va_list ap;
    va_start(ap,fmt); 
    
    vfprintf(stderr, fmt, ap);
    exit(-1);
}

float timedifference_msec(struct timeval t0, struct timeval t1)
{
    return (t1.tv_sec - t0.tv_sec) * 1000.0f + (t1.tv_usec - t0.tv_usec) / 1000.0f;
}

#define CASE(s) if (!strcmp(s, S_))

int main(int argc, char **argv)
{
    int argp = 1;
    bimg_t *bimg;
    int num_steps = 1;

    while(argp < argc && argv[argp][0] == '-') {
        char *S_ = argv[argp++];

        CASE("-help") {
            printf("morph - \n\n"
                   "Syntax:\n"
                   "    morph [] ...\n"
                   "\n"
                   "Options:\n"
                   "    -n n    Number of iterations\n");
            exit(0);
        }
        CASE("-n") {
            num_steps = atoi(argv[argp++]);
            continue;
        }
        die("Unknown option %s!\n", S_);
    }

    const char *filename;
    if (argp < argc)
        filename = argv[argp++];
    else
        die("Need png file!\n");
    bimg = bimg_new_from_png(filename);
    struct timeval t0;
    struct timeval t1;
    float elapsed;

    gettimeofday(&t0, 0);

    morph_erode_isotropic(bimg, num_steps);

    gettimeofday(&t1, 0);

    elapsed = timedifference_msec(t0, t1);

    printf("Code executed in %f milliseconds.\n", elapsed);
    bimg_save_png(bimg, "out.png");
    bimg_free(bimg);

    exit(0);
    return(0);
}
