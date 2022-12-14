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

config  *shm_cfg;
coord   *shm_ports_coords;
int     shm_id_config;
int     shm_id_ports_coords;
int     mq_id_request;
int     sem_id_generation;
int     id;

/* ? porto_goodsOffers_generator(); */

int main(int argc, char *argv[]) {
    int     i;
    long    n_docks;
    struct sigaction sa;
    struct sembuf sem;

    sa.sa_handler = porto_sig_handler;

    sigaction(SIGALRM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);

    srandom(getpid());

    if(argc != 5) {
        printf("Incorrect number of parameters [%d]. Exiting...\n", argc);
        kill(getppid(), SIGINT);
    }

    shm_id_config = string_to_int(argv[0]);
    shm_id_ports_coords = string_to_int(argv[1]);
    mq_id_request = string_to_int(argv[2]);
    sem_id_generation = string_to_int(argv[3]);
    id = string_to_int(argv[4]);

    if((shm_cfg = shmat(shm_id_config, NULL, SHM_RDONLY)) == (void*) -1) {
        perror("[PORTO] Error while trying to attach to configuration shared memory");
        kill(getppid(), SIGINT);
    }

    if((shm_ports_coords = shmat(shm_id_ports_coords, NULL, SHM_RDONLY)) == (void*) -1) {
        perror("[PORTO] Error while trying to attach to ports coordinates shared memory");
        kill(getppid(), SIGINT);
    }

    printf("[%d] coord.x: %f\tcoord.y: %f\n", getpid(), shm_ports_coords[id].x, shm_ports_coords[id].y);

    n_docks = random() % (shm_cfg->SO_BANCHINE) + 1;
    printf("Il porto: %d, ha: %ld banchine\n", getpid(), n_docks);
    /* n_ docks dovranno essere gestite come risorsa condivisa protetta da un semaforo */


    if(sem_cmd(sem_id_generation, 0, -1, 0) < 0) {
        perror("[PORTO] Error while trying to release sem_id_generation");
        kill(getppid(), SIGINT);
    }

    while (1) {
        /* Codice del porto da eseguire */
    }
}

void porto_sig_handler(int signum) {
    switch (signum) {
        case SIGINT:
            exit(EXIT_FAILURE);
        case SIGALRM:
            /*printf("PORTO\n");*/
            break;
    }
}
