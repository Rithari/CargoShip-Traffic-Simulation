#ifndef PROGETTOSO_UTILS_H
#define PROGETTOSO_UTILS_H

#include "master.h"

/* TODO: improve usage because if statement is a bs */
#define CHECK_ERROR(x, pid, str) if(x) { \
                            printf("ERROR IN LINE %d\n", __LINE__); \
                            perror(str);    \
                            kill(pid, SIGINT);  \
                        } else {} \
/* The below function is used to calculate the hours of the config
 * into minutes and seconds for the nanosleep */
struct timespec calculate_timeout(int hours, int day_length);
/* TODO: maybe useless function */
void timespec_sub(struct timespec*, struct timespec*, struct timespec*);
void print_config(config *cfg);

int string_to_int(char *s);

#endif /*PROGETTOSO_UTILS_H*/
