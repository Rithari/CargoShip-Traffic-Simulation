#include <stdio.h>
#include "utils/logger.h"

//Generic qsort
//https://www.geeksforgeeks.org/generic-implementation-of-quicksort-algorithm-in-c/

int main() {
    printf("Hello, World!\n");
    LOG_FINE("Hello, i'm a fine log ");
    LOG_INFO("Hello, i'm a info log");
    LOG_SEVERE("Hello, i'm a SEVERE log");
    printf("Hello, i'm a test");

    return 0;
}