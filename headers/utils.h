#ifndef PROGETTOSO_UTILS_H
#define PROGETTOSO_UTILS_H

#include "master.h"

#define CHECK_ERROR_CHILD(x, str)   if((x)) { \
                                        perror(str); \
                                        exit(EXIT_FAILURE); \
                                    } else {} \

#define min(a, b) ((a) > (b) ? (b) : (a))

void print_config(config *cfg);
void nanosleep_function(double time, char* str);
/* ordina in base al peso la merce */
int compare_goods_template(const void *g, const void *g1);

int string_to_int(char *s);

#endif /*PROGETTOSO_UTILS_H*/
