#ifndef PROGETTOSO_UTILS_H
#define PROGETTOSO_UTILS_H

#include "master.h"

void timespec_sub(struct timespec*, struct timespec*, struct timespec*);
void print_config(config *cfg);

/* convert an integer into a string*/
char* int_to_string(int num);

int string_to_int(char *s);

#endif /*PROGETTOSO_UTILS_H*/
