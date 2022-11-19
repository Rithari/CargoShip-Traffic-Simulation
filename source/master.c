#include "../headers/master.h"

int     SO_NAVI;
int     SO_PORTI;
int     SO_MERCI;
int     SO_SIZE;
int     SO_MIN_VITA;
int     SO_MAX_VITA;
double  SO_LATO;
int     SO_SPEED = 10;
int     SO_CAPACITY;
int     STEP;

const goods *goods_types;

void initialize_so_vars() {
    int i;
    goods *goods_types_tmp = (goods*) malloc(sizeof(goods) * SO_MERCI);

    srand(getpid());

    for(i = 0; i < SO_MERCI; i++) {
        goods_types_tmp[i].id = i;
        goods_types_tmp[i].ton = (int) rand() % SO_SIZE + 1;
        goods_types_tmp[i].lifespan = (int) (rand() % (SO_MAX_VITA - SO_MIN_VITA)) + SO_MIN_VITA;
    }

    goods_types = goods_types_tmp;
    free(goods_types_tmp);


}

int main(void) {
    char* arg[] = {
        PATH_NAVE,
        NULL,
    };

    /* nave.o path is in source  */
    execv("source/nave.o", arg);
    perror("execv");
    return 0;
}