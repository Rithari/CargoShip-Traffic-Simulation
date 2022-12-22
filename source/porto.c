#include "../headers/utils.h"
#include "../headers/master.h"
#include "../headers/common_ipcs.h"

/*
Richiesta di merci[] | MQ di richieste universale tra processi: (dobbiamo sapere il tipo, la quantità e il porto di appartenenza)
Int Lifespan -1 per le richieste perché non scadono e sono l’unico elemento che distingue le due MQ oltre a essere due MQ diverse
Array / struct di tratte disponibili (caricata dalla funzione apposita)

Funzione per caricare le proprie domande e offerte nelle rispettive MQ
Funzione/i per la creazione di tratte()
Funzione/i per la comunicazione con le queue()
Banchina: gestita come una risorsa condivisa protetta da un semaforo (n_docks)*/

/*L'handler riceve il segnale di creazione delle merci e invoca la funzione designata --> "start_of_goods_generation"*/
void porto_sig_handler(int);

config  *shm_cfg;
coord   *shm_ports_coords;
int     shm_id_config;
int     shm_id_ports_coords;
int     shm_id_goods_template;
int     mq_id_request;
int     sem_id_gen_precedence;
int     id;
/* COMMENTO PROVVISIORIO
goodsList start_of_goods_generation(void);
goodsOffers goodsOffers_struct_generation(int id, int tons, int lifespan);
goodsRequests goodsRequest_struct_generation(int id, int tons, pid_t affiliated);
void goodsOffers_generator(int *goodsSetOffers, int *lifespanArray, int offersValue, int offersLength);
void goodsRequest_generator(int *goodsSetRequests, int *lifespanArray, int requestValue, int requestsLength);
void add(goodsList myOffers, goodsOffers); <--- solo per test
*/

/* goodsList myOffers; */
config *shm_cfg;
goods *shm_goods_template;
int sem_id_dock;

/* ? porto_goodsOffers_generator(); */

int main(int argc, char *argv[]) {
    int     i;
    struct sigaction sa;
    struct sembuf sem;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = porto_sig_handler;
    sa.sa_flags = SA_RESTART;
    sigaction(SIGCONT, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGALRM, &sa, NULL);
    sa.sa_flags |= SA_NODEFER;
    sigaction(SIGUSR1, &sa, NULL);

    srandom(getpid());

    if(argc != 8) {
        printf("Incorrect number of parameters [%d]. Exiting...\n", argc);
        kill(getppid(), SIGINT);
    }

    shm_id_config = string_to_int(argv[1]);
    CHECK_ERROR(errno, getppid(), "[PORTO] Error while trying to convert shm_id_config")
    shm_id_ports_coords = string_to_int(argv[2]);
    CHECK_ERROR(errno, getppid(), "[PORTO] Error while trying to convert shm_id_ports_coords")
    shm_id_goods_template = string_to_int(argv[3]);
    CHECK_ERROR(errno, getppid(), "[PORTO] Error while trying to convert shm_id_ports_coords")
    mq_id_request = string_to_int(argv[4]);
    CHECK_ERROR(errno, getppid(), "[PORTO] Error while trying to convert mq_id_request")
    sem_id_gen_precedence = string_to_int(argv[5]);
    CHECK_ERROR(errno, getppid(), "[PORTO] Error while trying to convert sem_id_gen_precedence")
    sem_id_dock = string_to_int(argv[6]);
    CHECK_ERROR(errno, getppid(), "[PORTO] Error while trying to convert sem_id_dock")
    id = string_to_int(argv[7]);
    CHECK_ERROR(errno, getppid(), "[PORTO] Error while trying to convert id")


    CHECK_ERROR((shm_cfg = shmat(shm_id_config, NULL, SHM_RDONLY)) == (void*) -1, getppid(),
                "[PORTO] Error while trying to attach to configuration shared memory")
    CHECK_ERROR((shm_ports_coords = shmat(shm_id_ports_coords, NULL, SHM_RDONLY)) == (void*) -1, getppid(),
                "[PORTO] Error while trying to attach to ports coordinates shared memory")
    CHECK_ERROR((shm_goods_template = shmat(shm_id_goods_template, NULL, SHM_RDONLY)) == (void*) -1, getppid(),
                "[PORTO] Error while trying to attach to ports coordinates shared memory")
    CHECK_ERROR(sem_cmd(sem_id_gen_precedence, 0, -1, 0) < 0, getppid(),
                "[PORTO] Error while trying to release sem_id_gen_precedence")


    printf("[%d] coord.x: %f\tcoord.y: %f\n", getpid(), shm_ports_coords[id].x, shm_ports_coords[id].y);
    /*start_of_goods_generation();*/

    while (1) {
        /* Codice del porto da eseguire */
    }
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

goodsOffers goodsOffers_struct_generation(int id, int tons, int lifespan) {
    goodsOffers newOffers;
    newOffers.id = id;
    newOffers.tons = tons;
    newOffers.lifespan = lifespan;
    return newOffers;
}

goodsRequests goodsRequest_struct_generation(int id, int tons, pid_t affiliated) {
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
    int old_errno = errno;
    struct timespec swell_duration, rem;

    switch (signum) {
        case SIGCONT:
            break;
        case SIGINT:
            /* semctl(sem_id, 0, IPC_RMID); */
            exit(EXIT_FAILURE);
        case SIGALRM:
            /*TODO: dump stato attuale*/
            printf("[PORTO] DUMP PID: [%d] SIGALRM\n", getpid());
            break;
        case SIGUSR1:
            /* swell occurred */
            printf("[PORTO] SWELL: %d\n", getpid());

            swell_duration = calculate_timeout(shm_cfg->SO_SWELL_DURATION, shm_cfg->SO_DAY_LENGTH);

            while (nanosleep(&swell_duration, &rem)) {
                switch (errno) {
                    case EINTR:
                        swell_duration = rem;
                        continue;
                    default:
                        perror("[PORTO] Generic error while sleeping");
                        kill(getppid(), SIGINT);
                }
            }
            break;
        default:
            printf("[PORTO] Signal: %s\n", strsignal(signum));
            break;
    }

    errno = old_errno;
}
