#ifndef PROGETTOSO_MASTER_H
#define PROGETTOSO_MASTER_H

#define _POSIX_C_SOURCE 199309L
#define _GNU_SOURCE

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <time.h>
#include <errno.h>
#include "utils.h"

#define KEY_GOODS_TON 0x12345678

#ifdef NDEBUG
    #define PATH_NAVE   "release/nave.o"
    #define PATH_PORTO  "release/porto.o"
    #define PATH_MASTER "release/master.o"
    #define PATH_METEO  "release/meteo.o"
#else
    #define PATH_NAVE   "debug/nave.o"
    #define PATH_PORTO  "debug/porto.o"
    #define PATH_MASTER "debug/master.o"
    #define PATH_METEO  "debug/meteo.o"
#endif

/*TODO: pensare ad un quadtree?*/

extern int     SO_NAVI;
extern int     SO_PORTI;
extern int     SO_MERCI;
extern int     SO_SIZE;
extern int     SO_MIN_VITA;
extern int     SO_MAX_VITA;
extern double  SO_LATO;
extern int     SO_SPEED;
extern int     SO_CAPACITY;
extern int     STEP;

typedef struct {
    int id;
    int ton;
    int lifespan;
} goods;


typedef struct {
    double x;
    double y;
} coord;

extern const goods *goods_types;



#endif /*PROGETTOSO_MASTER_H*/
