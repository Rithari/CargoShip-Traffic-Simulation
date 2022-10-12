#ifndef OS_PROJECT_LOGGER_H
#define OS_PROJECT_LOGGER_H

/* Fine log useful for tiny info*/
void log_fine(const char*, const char*);

/* Info log for all basic debug utility*/
void log_info(const char*, const char*);

/* Severe log for all problems occurred during execution*/
void log_severe(const char*, const char*);

#ifdef DEBUG

#define LOG_FINE(p_Str)     log_fine(__FUNCTION__, p_Str);
#define LOG_INFO(p_Str)     log_info(__FUNCTION__, p_Str);
#define LOG_SEVERE(p_Str)   log_severe(__FUNCTION__, p_Str);

#else

#define LOG_FINE(p_Str)
#define LOG_INFO(p_Str)
#define LOG_SEVERE(p_Str)

#endif

#endif //OS_PROJECT_LOGGER_H