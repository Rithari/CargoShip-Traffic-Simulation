#ifndef PROGETTOSO_UTILS_H
#define PROGETTOSO_UTILS_H

#include "master.h"

#define CHECK_ERROR_CHILD(x, str)   if((x)) { \
                                        perror(str); \
                                        exit(EXIT_FAILURE); \
                                    } else {}\

void print_config(config *cfg);
void nanosleep_function(double time, char* str);

int string_to_int(char *s);

#endif /*PROGETTOSO_UTILS_H*/
