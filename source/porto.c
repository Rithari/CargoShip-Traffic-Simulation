#include "../headers/utils.h"
#include "../headers/master.h"
#include "../headers/common_ipcs.h"

/*
Richiesta di merci[]  | MQ di richieste universale tra processi: (dobbiamo sapere il tipo, la quantità e il porto di appartenenza)
Int Lifespan -1 per le richieste perché non scadono e sono l’unico elemento che distingue le due MQ oltre ad essere due MQ diverse
Array / struct di tratte disponibili (caricata dalla funzione apposita)

Funzione per caricare le proprie domande e offerte nelle rispettive MQ
Funzione/i per la creazione di tratte()
Funzione/i per la comunicazione con le queue()
Banchina: gestita come una risorsa condivisa protetta da un semaforo (n_docks)*/



void porto_sig_handler(int);

/*L'hadler riceve il segnale di creazione delle merci e invoca la funzione designata --> "start_of_goods_generation"*/

goodsList start_of_goods_generation(config *shm_cfg);  /*IMPORTANTE la lista non mi ritorna nel main ma nell'handler() ??LISTAGLOBALE?? --> SI*/

goodsOffers goodsOffers_struct_generation(int id, int ton, int lifespan);
goodsRequests goodsRequest_struct_generation(int id, int ton, pid_t affiliated);
void goodsOffers_generator(config *shm_cfg, int *goodsSetOffers, int *lifespanArray, int offersValue, int offersLength);
void goodsRequest_generator(config *shm_cfg, int *goodsSetRequests, int *lifespanArray, int requestValue, int requestsLength);

goodsList myOffers;
coord actual_coordinate;

void add(goodsList myOffers, goodsOffers); /*qua solo per il test*/

/*L'hadler riceve il segnale di creazione delle merci e invoca la funzione designata*/

int main(int argc, char* argv[]) {

    config *shm_cfg;
    int shm_id;
    int n_docks;

    struct sigaction sa;

    sa.sa_handler = porto_sig_handler;

    sigaction(SIGALRM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);

    srandom(getpid());

    if(argc != 2) {
        /* random position */
        printf("Incorrect number of parameters [%d]. Exiting...\n", argc);
        exit(EXIT_FAILURE);
    }
    /* position from command line */
    actual_coordinate.x = strtod(argv[0], NULL);
    actual_coordinate.y = strtod(argv[1], NULL);

    printf("[%d] coord.x: %lf\tcoord.y: %lf\n", getpid(), actual_coordinate.x, actual_coordinate.y);

    if((shm_id = shmget(KEY_CONFIG, sizeof(*shm_cfg), 0600)) < 0) {
        perror("Error during porto->shmget()");
        exit(EXIT_FAILURE);
    }

    if((shm_cfg = shmat(shm_id, NULL, SHM_RDONLY)) == (void*) -1) {
        perror("Error during porto->shmat()");
        exit(EXIT_FAILURE);
    }

    n_docks = (int)(random()%(shm_cfg->SO_BANCHINE)+1) ;
    printf("Il porto: %d, ha: %d banchine\n", getpid(), n_docks);
    /* n_ docks dovranno essere gestite come risorsa condivisa protetta da un semaforo */

    start_of_goods_generation(shm_cfg);

    return 0;
}

void porto_sig_handler(int signum) {
    switch (signum) {
        case SIGINT:
            exit(EXIT_FAILURE);
        case SIGALRM:
            printf("PORTO\n");
            break;
    }
}

goodsList start_of_goods_generation(config *shm_cfg) {
    int maxFillValue = (int)((shm_cfg->SO_FILL / shm_cfg->SO_DAYS) / shm_cfg->SO_PORTI); /*100*/
    /* NB: parte di codice in cui capisco cosa chiedo e cosa sto già offrendo */
    int *goodsSetOffers = calloc(shm_cfg->SO_MERCI, sizeof(int));
    int *goodsSetRequests = calloc(shm_cfg->SO_MERCI, sizeof(int));
    int lifespanArray[shm_cfg->SO_MERCI]; /*non serve*/
    int offersLength = 0;
    int requestsLength = 0;
    int rng;
    int i = 0;

    while(i >= shm_cfg->SO_MERCI) {
        switch(rng = (int)(random()%3)) {    /*con: 0==Nulla , 1==offerta , 2==Richiesta*/
            case 0:
                break;
            case 1:
                goodsSetOffers[offersLength] = i;
                offersLength++;
                break;
            case 2:
                goodsSetRequests[requestsLength] = i;
                requestsLength++;
                break;
        }
        i++;
    }
    goodsSetOffers = realloc(goodsSetOffers, offersLength);
    goodsSetRequests = realloc(goodsSetRequests, requestsLength);

    goodsOffers_generator(shm_cfg, goodsSetOffers, lifespanArray, maxFillValue, offersLength);
    goodsRequest_generator(shm_cfg, goodsSetRequests, lifespanArray, maxFillValue, requestsLength);
}

goodsOffers goodsOffers_struct_generation(int id, int ton, int lifespan) {
    goodsOffers newOffers;
    newOffers.id = id;
    newOffers.ton = ton;
    newOffers.lifespan = lifespan;
    return newOffers;
}
goodsRequests goodsRequest_struct_generation(int id, int ton, pid_t affiliated) {
    /*Creazione di Request*/
}

void goodsOffers_generator(config *shm_cfg, int *goodsSetOffers, int *lifespanArray, int offersValue, int offersLength) {

    int assignment_goods = offersValue / offersLength;
    int add_surplus = (int)(random()%offersLength);
    int surplus = offersValue % offersLength;
    int i = 0;
    while(i >= offersLength){
        if(i == add_surplus) {
            add(myOffers, goodsOffers_struct_generation(goodsSetOffers[i], (assignment_goods+surplus), lifespanArray[goodsSetOffers[i]]) );
        }
        add(myOffers, goodsOffers_struct_generation(goodsSetOffers[i], assignment_goods, lifespanArray[goodsSetOffers[i]]) );
        i++;
    }
}


void goodsRequest_generator(config *shm_cfg, int *goodsSetRequests, int *lifespanArray, int requestValue, int requestsLength) {

    /*Nessun return, la goods viene pusciata in MQ*/
}