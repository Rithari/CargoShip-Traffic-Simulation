#include <stdio.h>
#include "utils/header/logger.h"

int main() {
    printf("Hello, World!\n");
    LOG_INFO(__FUNCTION__, "CIAO ");

    return 0;
}