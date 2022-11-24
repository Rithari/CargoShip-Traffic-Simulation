#include "../headers/utils.h"

void timespec_sub(struct timespec* res, struct timespec *minuend, struct timespec *subtrahend) {
    res->tv_sec = minuend->tv_sec - subtrahend->tv_sec;
    res->tv_nsec = minuend->tv_nsec - subtrahend->tv_nsec;
}

void print_config(config *cfg) {
    printf("--------------------------\n");
    printf("--------- CONFIG ---------\n");
    printf("--------------------------\n");
    printf("SO_NAVI:        %d\n", cfg->SO_NAVI);
    printf("SO_PORTI:       %d\n", cfg->SO_PORTI);
    printf("SO_MERCI:       %d\n", cfg->SO_MERCI);
    printf("SO_SIZE:        %d\n", cfg->SO_SIZE);
    printf("SO_MIN_VITA:    %d\n", cfg->SO_MIN_VITA);
    printf("SO_MAX_VITA:    %d\n", cfg->SO_MAX_VITA);
    printf("SO_LATO:        %f\n", cfg->SO_LATO);
    printf("SO_SPEED:       %d\n", cfg->SO_SPEED);
    printf("SO_CAPACITY:    %d\n", cfg->SO_CAPACITY);
    printf("SO_BANCHINE:    %d\n", cfg->SO_BANCHINE);
    printf("SO_FILL:        %d\n", cfg->SO_FILL);
    printf("SO_LOADSPEED:   %d\n", cfg->SO_LOADSPEED);
    printf("STEP:           %d\n", cfg->STEP);
    printf("SO_DAYS:        %d\n", cfg->SO_DAYS);
    printf("TOPK:           %d\n", cfg->TOPK);
    printf("STORM_DURATION: %d\n", cfg->STORM_DURATION);
    printf("SWELL_DURATION: %d\n", cfg->SWELL_DURATION);
    printf("ML_INTENSITY:   %d\n", cfg->ML_INTENSITY);
#ifdef DEBUG
    printf("--------- DEBUG  ---------\n");
    printf("CHECK:          %d\n", cfg->check);
#endif
    printf("--------------------------\n");
}
