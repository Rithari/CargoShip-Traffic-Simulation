//
// Created by mattia on 11/10/22.
//

#ifndef PROGETTO_SO_LOGGER_H
#define PROGETTO_SO_LOGGER_H

void log_fine(const char*, const char*);
void log_info(const char*, const char*);
void log_severe(const char*, const char*);

#ifdef DEBUG

#define LOG_FINE(p_FunctionName, p_Str)     log_fine(p_FunctionName, p_Str);
#define LOG_INFO(p_FunctionName, p_Str)     log_info(p_FunctionName, p_Str);
#define LOG_SEVERE(p_FunctionName, p_Str)   log_severe(p_FunctionName, p_Str);

#else

#define LOG_FINE(p_FunctionName, p_Str)
#define LOG_INFO(p_FunctionName, p_Str)
#define LOG_SEVERE(p_FunctionName, p_Str)

#endif

#endif //PROGETTO_SO_LOGGER_H
