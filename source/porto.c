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

/* ? porto_goodsOffers_generator(); */

int main(int argc, char *argv[]) {
    coord actual_coordinate;
    config *shm_cfg;
    int shm_id;
    long n_docks;

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

    n_docks = random() % (shm_cfg->SO_BANCHINE) + 1;
    printf("Il porto: %d, ha: %ld banchine\n", getpid(), n_docks);
    /* n_ docks dovranno essere gestite come risorsa condivisa protetta da un semaforo */


    return 0;
}
