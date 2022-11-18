#include "../headers/utils.h"

void timespec_sub(struct timespec* res, struct timespec *minuend, struct timespec *subtrahend) {
    res->tv_sec = minuend->tv_sec - subtrahend->tv_sec;
    res->tv_nsec = minuend->tv_nsec - subtrahend->tv_nsec;
}

