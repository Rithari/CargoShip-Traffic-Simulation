#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include "master.h"

#define _GNU_SOURCE

#ifdef RELEASE
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

int     SO_NAVI;
int     SO_PORTI;
int     SO_MERCI;
int     SO_SIZE;
int     SO_MIN_VITA;
int     SO_MAX_VITA;
double  SO_LATO;
int     SO_SPEED;
int     SO_CAPACITY;
int     STEP;

goods *goods_types;

void initialize_so_vars() {
    int i;
    goods_types = (goods*) malloc(sizeof(goods) * SO_MERCI);

    srand(getpid());

    for(i = 0; i < SO_MERCI; i++) {
        goods_types[i].id = i;
        goods_types[i].dim = (int) rand() % SO_SIZE + 1;
        goods_types[i].lifespan = (int) (rand() % (SO_MAX_VITA - SO_MIN_VITA)) + SO_MIN_VITA;
    }
}

int main(void) {
    return 0;
}