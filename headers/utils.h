#ifndef PROGETTOSO_UTILS_H
#define PROGETTOSO_UTILS_H

#include "master.h"

#define CHECK_ERROR_CHILD(x, str)  if((x)) { \
                                            perror(str); \
                                            exit(EXIT_FAILURE); \
                                        }
/* The below function is used to calculate the hours of the config
 * into minutes and seconds for the nanosleep */
struct timespec calculate_timeout(int hours, int day_length);
/* TODO: maybe useless function */
void timespec_sub(struct timespec*, struct timespec*, struct timespec*);
void print_config(config *cfg);

int string_to_int(char *s);

#endif /*PROGETTOSO_UTILS_H*/
