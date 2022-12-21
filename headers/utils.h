#ifndef PROGETTOSO_UTILS_H
#define PROGETTOSO_UTILS_H

#include "master.h"

/* TODO: check relevance */
#define CHECK_ERROR(x, pid, str) if(x) { \
                            printf("ERROR IN LINE %d\n", __LINE__); \
                            perror(str);    \
                            kill(pid, SIGINT);  \
                        }
struct timespec calculate_timeout(int hours, int day_length);
/* TODO: maybe useless function */
void timespec_sub(struct timespec*, struct timespec*, struct timespec*);
void print_config(config *cfg);

/* convert an integer into a string*/
char* int_to_string(int num);

int string_to_int(char *s);

#endif /*PROGETTOSO_UTILS_H*/
