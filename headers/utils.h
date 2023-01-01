#ifndef PROGETTOSO_UTILS_H
#define PROGETTOSO_UTILS_H

#include "master.h"

#define CHECK_ERROR_CHILD(x, str)  do { \
                                        if((x)) { \
                                            perror(str); \
                                            exit(EXIT_FAILURE); \
                                        } \
                                    } while (0);

void print_config(config *cfg);
struct timespec calculate_sleep_time(double x);

int string_to_int(char *s);

#endif /*PROGETTOSO_UTILS_H*/
