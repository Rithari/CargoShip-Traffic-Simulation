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

config  *shm_cfg;
coord   *shm_ports_coords;
generalGoods *shm_goods_template;
dump_ports  *shm_dump_ports;
dump_goods  *shm_dump_goods;
int     shm_id_config;
int     myid;
int     mqRequests;
int    *arrayDecision_making;
int     leftoverOffers = 0;
int     leftoverRequests = 0;
int    *shm_mq_ids;

void porto_sig_handler(int);

void start_of_goods_generation(void);
void goodsOffers_generator(int id, int quantity, int lifespan);
void goodsRequest_generator(int id, int quantity, int affiliated);

int main(int argc, char *argv[]) {
    struct sigaction sa;
    struct sembuf sem;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = porto_sig_handler;
    sa.sa_flags = SA_RESTART;
    sigaction(SIGCONT, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGALRM, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sa.sa_flags |= SA_NODEFER;
    sigaction(SIGUSR1, &sa, NULL);

    srandom(getpid());

    if(argc != 3) {
        printf("Incorrect number of parameters [%d]. Exiting...\n", argc);
        kill(getppid(), SIGINT);
    }

    /*TODO: improve string to int function */

    shm_id_config = string_to_int(argv[1]);
    CHECK_ERROR_CHILD(errno, "[PORTO] Error while trying to convert shm_id_config")
    myid = string_to_int(argv[2]);
    CHECK_ERROR_CHILD(errno, "[PORTO] Error while trying to convert id")

    CHECK_ERROR_CHILD((shm_cfg = shmat(shm_id_config, NULL, SHM_RDONLY)) == (void*) -1,
                      "[PORTO] Error while trying to attach to configuration shared memory")
    CHECK_ERROR_CHILD((shm_ports_coords = shmat(shm_cfg->shm_id_ports_coords, NULL, SHM_RDONLY)) == (void*) -1,
                      "[PORTO] Error while trying to attach to ports coordinates shared memory")
    CHECK_ERROR_CHILD((shm_mq_ids = shmat(shm_cfg->shm_id_mq_offer, NULL, 0)) == (void*) -1,
                      "[PORTO] Error while trying to attach to array of mq id shared memory")
    CHECK_ERROR_CHILD((shm_goods_template = shmat(shm_cfg->shm_id_goods_template, NULL, SHM_RDONLY)) == (void*) -1,
                      "[PORTO] Error while trying to attach to ports coordinates shared memory")
    CHECK_ERROR_CHILD((shm_dump_ports = shmat(shm_cfg->shm_id_dump_ports, NULL, 0)) == (void*) -1,
                      "[PORTO] Error while trying to attach to dump port shared memory")
    CHECK_ERROR_CHILD((shm_dump_goods = shmat(shm_cfg->shm_id_dump_goods, NULL, 0)) == (void*) -1,
                      "[PORTO] Error while trying to attach to dump goods shared memory")
    CHECK_ERROR_CHILD(sem_cmd(shm_cfg->sem_id_gen_precedence, 0, -1, 0) < 0,
                      "[PORTO] Error while trying to release sem_id_gen_precedence")
    CHECK_ERROR_CHILD((shm_mq_ids[myid] = msgget(IPC_PRIVATE, 0600)) < 0,
                      "[PORTO] Error while trying to create my mq")

    fflush(stdout);


    arrayDecision_making = calloc(shm_cfg->SO_MERCI, sizeof(int));
    int i;
    for (i = 0; i<shm_cfg->SO_MERCI; i++) {
        arrayDecision_making[i] = (int)(random() % 3);   // 0=nulla - 1=offre - 2=richiede
    }

    /*printf("[%d] coord.x: %f\tcoord.y: %f\n", getpid(), shm_ports_coords[id].x, shm_ports_coords[id].y);
    start_of_goods_generation();*/

    while (1) {
        /* Codice del porto da eseguire */
        pause();
    }
}


/* COME STRUTTURARE LA FUNZIONE DELLA GENERAZIONE DELLE MERCI
*  0) La funzione di generazione delle merci e' chiaramente suddivisa nella parte delle offerte e delle richieste.
 * 1) Usiamo una linked list per le offerte, per ogni porto. (Ogni porto ha la sua, e, le richieste vanno in MQ)
 * 2) La linked list è composta da un nodo che contiene la struct di tipo goods delle offerte e un puntatore al nodo successivo
 * 3) La struct delle offerte contiene:
 *     - id della merce
 *     - tons (quantita')
 *     - lifespan (durata)
 * 4) La linked list viene ordinata in base al lifespan, dal piu corto al piu lungo.
 *      Ogni giorno, il porto controlla se il lifespan e' scaduto, se si, elimina la merce dalla lista finche' non trova una merce con lifespan > 0
 *      Se il lifespan non e' scaduto, decrementa il lifespan di 1.
 * 5) La funzione come prima cosa prende SO_FILL, SO_DAYS e SO_PORTI e suddivide le quantita' massima per ogni porto, ogni giorno.
 * 6) Per ogni porto (ovvero quello che chiama la funzione) genera un numero compreso nel limite sovra citato, e lo suddivide in offerte e richieste.
 * 7) Per ogni offerta e richiesta genera un id, un tons e un lifespan. Lifespan per la richiesta e' -1, per l'offerta e' random.
 * 8) Per ogni offerta generata, la inserisce nella linked list, in base al lifespan.
 * 9) Per ogni richiesta generata, la inserisce nella MQ.
 *10) Passiamo i puntatori alla linked list alla funzione di generazione delle offerte.
 * La linked list ovviamente e' una variabile globale.
 */


void start_of_goods_generation(void) { //quanto posso generare, cosa va in o e cosa in r, il mio pid le struct
    int maxFillValue = (int)((shm_cfg->SO_FILL / shm_cfg->SO_DAYS) / shm_cfg->SO_PORTI); //con questo ho so_fill per p
    int maxFillOffers = maxFillValue + leftoverOffers;
    int maxFillRequests = maxFillValue + leftoverRequests;

    int rng;
    int iton;
    int imaxQuantity;
    int i = 0;
    while(i < shm_cfg->SO_MERCI) {
        switch (arrayDecision_making[i]) {
            case 0:
                break;

            case 1: //offerta
                iton = shm_goods_template[i].ton;
                if(iton <= maxFillOffers) {
                    imaxQuantity = (int)(maxFillOffers/iton);
                    rng = (int)(random() % imaxQuantity);
                    maxFillOffers = maxFillOffers - (rng*iton);
                    goodsOffers_generator(i , rng , (shm_goods_template[i].lifespan + shm_cfg->CURRENT_DAY));
                }
                break;

            case 2: //richiesta
                iton = shm_goods_template[i].ton;
                if(iton <= maxFillRequests) {
                    imaxQuantity = (int)(maxFillRequests/iton);
                    rng = (int)(random() % imaxQuantity);
                    maxFillRequests = maxFillRequests - (rng*iton);
                    goodsRequest_generator(i , rng , myid);
                }
                break;
        }
        i++;
    }
    leftoverOffers = maxFillOffers;
    leftoverRequests = maxFillRequests;
}


void goodsOffers_generator(int id, int quantity, int lifespan) {
    offerMessage newOffer;
    newOffer.mtype = id;
    newOffer.quantity = quantity;
    newOffer.lifespan = lifespan;
    msgsnd(shm_mq_ids[id], &newOffer, sizeof(newOffer), 0600);
}

void goodsRequest_generator(int id, int quantity, int affiliated) {
    requestMessage newRequest;
    newRequest.mtype = id;
    newRequest.quantity = quantity;
    newRequest.requestingPort = affiliated;
    msgsnd(shm_cfg->mq_id_request, &newRequest, sizeof(newRequest), 0600);
}


void porto_sig_handler(int signum) {
    int old_errno = errno;
    struct timespec swell_duration, rem;

    switch (signum) {
        case SIGTERM:
            exit(EXIT_SUCCESS);
        case SIGCONT:
            break;
        case SIGINT:
            exit(EXIT_FAILURE);
        case SIGALRM:
            /*TODO: dump stato attuale*/
            /*printf("[PORTO] DUMP PID: [%d] SIGALRM\n", getpid());*/
            shm_dump_ports[myid].id = myid;
            /* TODO: bug nell'otterenere il valore attuale del semaforo sem_id_dock[id] */
            CHECK_ERROR_CHILD((shm_dump_ports[myid].dock_available = semctl(shm_cfg->sem_id_dock, myid, GETVAL)),
                              "[PORTO] Error while trying to get dock available dock")
            CHECK_ERROR_CHILD(sem_cmd(shm_cfg->sem_id_gen_precedence, 0, -1, 0) < 0,
                              "[PORTO] Error while trying to release sem_id_gen_precedence")
            break;
        case SIGUSR1:
            /* swell occurred */
            /*printf("[PORTO] SWELL: %d\n", getpid()); */

            swell_duration = calculate_timeout(shm_cfg->SO_SWELL_DURATION, shm_cfg->SO_DAY_LENGTH);
            shm_dump_ports[myid].on_swell = 1;
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
            shm_dump_ports[myid].on_swell = 0;
            break;
        default:
            printf("[PORTO] Signal: %s\n", strsignal(signum));
            break;
    }

    errno = old_errno;
}
