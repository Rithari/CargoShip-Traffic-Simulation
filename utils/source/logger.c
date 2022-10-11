//
// Created by mattia on 11/10/22.
//

#include <stdio.h>
#include "../header/logger.h"

void log_fine(const char* p_FunctionName, const char* p_Msg) {
    printf("\tFUNCTION: %s\n\tFINE: %s", p_FunctionName, p_Msg);
}
void log_info(const char* p_FunctionName, const char* p_Msg) {
    printf("\tFUNCTION: %s\n\tINFO: %s", p_FunctionName, p_Msg);
}
void log_severe(const char* p_FunctionName, const char* p_Msg) {
    printf("\tFUNCTION: %s\n\tSEVERE: %s", p_FunctionName, p_Msg);
}