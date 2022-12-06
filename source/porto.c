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

/*L'handler riceve il segnale di creazione delle merci e invoca la funzione designata --> "start_of_goods_generation"*/
void porto_sig_handler(int);

/* COMMENTO PROVVISIORIO
goodsList start_of_goods_generation(void);
goodsOffers goodsOffers_struct_generation(int id, int ton, int lifespan);
goodsRequests goodsRequest_struct_generation(int id, int ton, pid_t affiliated);
void goodsOffers_generator(int *goodsSetOffers, int *lifespanArray, int offersValue, int offersLength);
void goodsRequest_generator(int *goodsSetRequests, int *lifespanArray, int requestValue, int requestsLength);
void add(goodsList myOffers, goodsOffers); <--- solo per test
*/

goodsList myOffers;
config *shm_cfg;

int sem_id;
coord actual_coordinates;

int main(int argc, char *argv[]) {
    int shm_id;
    int n_docks;
    struct sigaction sa;
    int key = KEY_SEM + getpid();
    int i;


    sa.sa_handler = porto_sig_handler;
    sa.sa_flags = SA_RESTART;
    sigaction(SIGALRM, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);

    srandom(getpid());

    if(argc != 2) {
        printf("Incorrect number of parameters [%d]. Exiting...\n", argc);
        exit(EXIT_FAILURE);
    }

    /* position from command line */
    actual_coordinates.x = strtod(argv[0], NULL);
    actual_coordinates.y = strtod(argv[1], NULL);

    /* printf("[%d] coord.x: %lf\tcoord.y: %lf\n", getpid(), actual_coordinates.x, actual_coordinates.y); */

    if((shm_id = shmget(KEY_CONFIG, sizeof(*shm_cfg), 0600)) < 0) {
        perror("Error during porto->shmget()");
        exit(EXIT_FAILURE);
    }

    if((shm_cfg = shmat(shm_id, NULL, SHM_RDONLY)) == (void*) -1) {
        perror("Error during porto->shmat()");
        exit(EXIT_FAILURE);
    }

    n_docks = (int)random() % (shm_cfg->SO_BANCHINE) + 1;
    sem_id = initialize_semaphore(key, n_docks);

    /* Find this port in the array of port coordinates, and save the semaphore id */
    for(i = 0; i < shm_cfg->SO_PORTI; i++) {
        if(ports_coords[i].x == actual_coordinates.x && ports_coords[i].y == actual_coordinates.y) {
            ports_coords[i].semaphore_id = sem_id;
            break;
        }
    }

    /*start_of_goods_generation();*/

    while (1) {
        /* Codice del porto da eseguire */
    }

    return 0;
}

/* COMMENTO PROVVISORIO WIP
goodsList start_of_goods_generation(void) {
    int maxFillValue = (int)((shm_cfg->SO_FILL / shm_cfg->SO_DAYS) / shm_cfg->SO_PORTI); 100
    < NB: parte di codice in cui capisco cosa chiedo e cosa sto già offrendo >
    int *goodsSetOffers = calloc(shm_cfg->SO_MERCI, sizeof(int));
    int *goodsSetRequests = calloc(shm_cfg->SO_MERCI, sizeof(int));
    int lifespanArray[shm_cfg->SO_MERCI]; <---- non serve
    int offersLength = 0;
    int requestsLength = 0;
    int rng;
    int i = 0;

    while(i >= shm_cfg->SO_MERCI) {
        switch(rng = (int)(random()%3)) {    con: 0==Nulla , 1==offerta , 2==Richiesta
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

    goodsOffers_generator(goodsSetOffers, lifespanArray, maxFillValue, offersLength);
    goodsRequest_generator(goodsSetRequests, lifespanArray, maxFillValue, requestsLength);
}

goodsOffers goodsOffers_struct_generation(int id, int ton, int lifespan) {
    goodsOffers newOffers;
    newOffers.id = id;
    newOffers.ton = ton;
    newOffers.lifespan = lifespan;
    return newOffers;
}

goodsRequests goodsRequest_struct_generation(int id, int ton, pid_t affiliated) {
    ..Creazione di Request...
}

void goodsOffers_generator(int *goodsSetOffers, int *lifespanArray, int offersValue, int offersLength) {

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


void goodsRequest_generator(int *goodsSetRequests, int *lifespanArray, int requestValue, int requestsLength) {

    ..Nessun return, la goods viene pusciata in MQ..
} */

void porto_sig_handler(int signum) {
    switch (signum) {
        case SIGTERM:
        case SIGINT:
            semctl(sem_id, 0, IPC_RMID);
            exit(EXIT_FAILURE);
        case SIGALRM:
            printf("PORTO\n");
            break;
        default:
            break;
    }
}
