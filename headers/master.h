#ifndef PROGETTOSO_MASTER_H
#define PROGETTOSO_MASTER_H

#define _POSIX_C_SOURCE 199309L
#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/stat.h>

#define EXIT_DEATH      127

#ifdef NDEBUG
#define PATH_NAVE   "release/nave.out"
    #define PATH_PORTO  "release/porto.out"
    #define PATH_MASTER "release/master.out"
    #define PATH_METEO  "release/meteo.out"
#else
#define PATH_NAVE   "debug/nave.out"
#define PATH_PORTO  "debug/porto.out"
#define PATH_MASTER "debug/master.out"
#define PATH_METEO  "debug/meteo.out"
#endif


typedef struct {
    int id;
    int ton;
    int lifespan;
} goods;


typedef struct {
    double x;
    double y;
} coord;

typedef struct {
    int     CURRENT_DAY;
    int     SO_NAVI;
    int     SO_PORTI;
    int     SO_MERCI;
    int     SO_SIZE;
    int     SO_MIN_VITA;
    int     SO_MAX_VITA;
    double  SO_LATO;
    int     SO_SPEED;
    int     SO_CAPACITY;
    int     SO_BANCHINE;
    int     SO_FILL;
    int     SO_LOADSPEED;
    int     SO_DAYS;
    int     SO_DAY_LENGTH;
    int     STORM_DURATION;
    int     SWELL_DURATION;
    int     ML_INTENSITY;
    unsigned int check;
} config;

#endif /*PROGETTOSO_MASTER_H*/
