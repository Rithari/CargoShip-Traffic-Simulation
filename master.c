#include <stdio.h>

/*Generic qsort
https://www.geeksforgeeks.org/generic-implementation-of-quicksort-algorithm-in-c*/

#include <stdlib.h>
#include <unistd.h>
#include "master.h"

#define _GNU_SOURCE

int SO_NAVI;
int SO_PORTI;
int SO_MERCI;
int SO_SIZE;
int SO_MIN_VITA;
int SO_MAX_VITA;
double SO_LATO;
int SO_SPEED;
int SO_CAPACITY;
int STEP;

goods *goods_types;

void initialize_so_vars() {
    int i;
    goods_types = (goods*) malloc(sizeof(goods) * SO_MERCI);

    srandom(getpid());

    for(i = 0; i < SO_MERCI; i++) {
        goods_types[i].id = i;
        goods_types[i].dim = (int) random() % SO_SIZE + 1;
        goods_types[i].lifespan = (int) (random() % (SO_MAX_VITA - SO_MIN_VITA)) + SO_MIN_VITA;
    }
}

int main() {
    execlp("nave.out", "nave.out", NULL);
    perror("");
    return 0;
}