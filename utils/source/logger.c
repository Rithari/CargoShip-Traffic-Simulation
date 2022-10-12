#include <stdio.h>
#include "../logger.h"

void log_fine(const char* p_FunctionName, const char* p_Msg) {
    printf("\033[0;36mFUNCTION: %s\nFINE: %s\033[0m\n", p_FunctionName, p_Msg);
}

void log_info(const char* p_FunctionName, const char* p_Msg) {
    printf("\033[0;34mFUNCTION: %s\nINFO: %s\033[0m\n", p_FunctionName, p_Msg);
}

void log_severe(const char* p_FunctionName, const char* p_Msg) {
    printf("\033[1;31mFUNCTION: %s\nSEVERE: %s\033[0m\n", p_FunctionName, p_Msg);
}