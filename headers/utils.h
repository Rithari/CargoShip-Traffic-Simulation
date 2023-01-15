#ifndef PROGETTOSO_UTILS_H
#define PROGETTOSO_UTILS_H

#include "master.h"

/* MACRO for error checking an expression. If it evaluates to true the error is printed and the process exits */
#define CHECK_ERROR_CHILD(x, str)   if((x)) { \
                                        perror(str); \
                                        exit(EXIT_FAILURE); \
                                    } else {} \

/* Math doesn't have a MIN function for some reason, so it's defined in this macro instead. */
#define min(a, b) ((a) > (b) ? (b) : (a))

void print_config(config *cfg);
void sleep_ns(double time, char* str); /* Function to sleep for a given amount of time */
int compare_goods_template(const void *g, const void *g1);

int string_to_int(char *s);

#endif /*PROGETTOSO_UTILS_H*/
