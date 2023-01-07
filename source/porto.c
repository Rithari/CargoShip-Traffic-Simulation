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
int generate_goods(int);
int generate_route(void);

        config  *shm_cfg;
coord   *shm_ports_coords;
goods_template   *shm_goods_template;
int*    shm_goods;
pid_t   *shm_pid_array;
dump_ports  *shm_dump_ports;
dump_goods  *shm_dump_goods;

int     shm_id_config;
int     id;
int     ton_in_excess;

int main(int argc, char *argv[]) {
    struct sigaction sa;
    msg_handshake msg;

    srandom(getpid());

    if(argc != 3) {
        printf("Incorrect number of parameters [%d]. Exiting...\n", argc);
        kill(getppid(), SIGINT);
    }

    shm_id_config = string_to_int(argv[1]);
    CHECK_ERROR_CHILD(errno, "[PORTO] Error while trying to convert shm_id_config")
    id = string_to_int(argv[2]);
    CHECK_ERROR_CHILD(errno, "[PORTO] Error while trying to convert id")

    CHECK_ERROR_CHILD((shm_cfg = shmat(shm_id_config, NULL, SHM_RDONLY)) == (void*) -1,
                      "[PORTO] Error while trying to attach to configuration shared memory")
    CHECK_ERROR_CHILD((shm_pid_array = shmat(shm_cfg->shm_id_pid_array, NULL, 0)) == (void*) -1,
                      "[PORTO] Error while trying to attach to pid_array shared memory");
    CHECK_ERROR_CHILD((shm_goods = shmat(shm_cfg->shm_id_goods, NULL, 0)) == (void*) -1,
                      "[PORTO] Error while trying to attach to goods shared memory");
    CHECK_ERROR_CHILD((shm_ports_coords = shmat(shm_cfg->shm_id_ports_coords, NULL, SHM_RDONLY)) == (void*) -1,
                      "[PORTO] Error while trying to attach to ports coordinates shared memory")
    CHECK_ERROR_CHILD((shm_goods_template = shmat(shm_cfg->shm_id_goods_template, NULL, SHM_RDONLY)) == (void*) -1,
                      "[PORTO] Error while trying to attach to goods_template shared memory")
    CHECK_ERROR_CHILD((shm_dump_ports = shmat(shm_cfg->shm_id_dump_ports, NULL, 0)) == (void*) -1,
                      "[PORTO] Error while trying to attach to dump port shared memory")
    CHECK_ERROR_CHILD((shm_dump_goods = shmat(shm_cfg->shm_id_dump_goods, NULL, 0)) == (void*) -1,
                      "[PORTO] Error while trying to attach to dump goods_template shared memory")

    shm_dump_ports[id].dock_total = (int) random() % shm_cfg->SO_BANCHINE + 1;
    CHECK_ERROR_CHILD(semctl(shm_cfg->sem_id_dock, id, SETVAL, shm_dump_ports[id].dock_total),
                      "[PORTO] Error while generating dock semaphore")

    /* printf("[%d] coord.x: %f\tcoord.y: %f\n", getpid(), shm_ports_coords[id].x, shm_ports_coords[id].y); */

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = porto_sig_handler;

    sigaction(SIGCONT, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGUSR2, &sa, NULL);
    sa.sa_flags |= SA_NODEFER;
    sigaction(SIGUSR1, &sa, NULL);
    sigaddset(&sa.sa_mask, SIGUSR1);
    sigaction(SIGALRM, &sa, NULL);

    CHECK_ERROR_CHILD(sem_cmd(shm_cfg->sem_id_gen_precedence, 0, -1, 0) < 0,
                      "[PORTO] Error while trying to release sem_id_gen_precedence")

    pause();

    while (1) {
        /* Codice del porto da eseguire */
        while (msgrcv(shm_cfg->mq_id_ports_handshake, &msg, sizeof(msg.response_pid), id + 1, 0) < 0) {
            CHECK_ERROR_CHILD(errno != EINTR, "[PORTO] Error while waiting handshake message")
        }
        printf("[%d] Received message from [%d]\n", getpid(), msg.response_pid);
        msg.mtype = msg.response_pid;
        msg.response_pid = generate_route();
        while (msgsnd(shm_cfg->mq_id_ships_handshake, &msg, sizeof(msg.response_pid), 0)) {
            CHECK_ERROR_CHILD(errno != EINTR, "[PORTO] Error while sending handshake message")
        }
        printf("[%d] Ok given to [%ld]\n", getpid(), msg.mtype);
    }
}

int generate_goods(int excess) {
    int i, j;
    int selected_quantity;
    int tons_per_port = shm_cfg->SO_FILL / shm_cfg->SO_DAYS / shm_cfg->GENERATING_PORTS + excess;

    for(i = 0, j = (int) random() % shm_cfg->SO_MERCI; i < shm_cfg->SO_MERCI; i++, j = (j + 1) % shm_cfg->SO_MERCI) {
        int max_quantity = tons_per_port / shm_goods_template[j].tons;

        if (max_quantity) {
            selected_quantity = (int) random() % max_quantity;

            if (selected_quantity) {
                if (shm_goods[id + j] == 0) {
                    shm_goods[id + j] = random() & 1 ? selected_quantity : -selected_quantity;
                } else if (shm_goods[id + j] > 0) {
                    shm_goods[id + j] += selected_quantity;
                } else {
                    shm_goods[id + j] -= selected_quantity;
                }
                /*TODO: ll item*/
                tons_per_port -= (selected_quantity * shm_goods_template[j].tons);
            }
        }
    }
    return tons_per_port;
}

int generate_route(void) {
    int i, j, min_val;
    int best_route = -1;
    int best_tons_available = shm_cfg->SO_CAPACITY;
    int ship_tons_available;
    int *tmp_goods_to_get = malloc(sizeof(int) * shm_cfg->SO_MERCI);
    int *goods_to_get = malloc(sizeof(int) * shm_cfg->SO_MERCI);
    int offset;
    int best_offset = INT_MAX;

    for (i = 0; i < shm_cfg->SO_MERCI; i++) {
        if (shm_goods_template[i].tons < best_offset) {
            best_offset = shm_goods_template[i].tons;
        }
    }

    /* TODO: ottimizzazioni possibili: randomizzare i per avere una maggiore distribuzione delle navi */

    for(i = 0; i < shm_cfg->SO_PORTI && best_tons_available > best_offset; i++) {
        if(id != i) {
            /* calcolo la quantità minore di merce che posso caricare attualmente,
             * se la nave ha capienza rimanente minore non ha senso iterare */
            for (j = 0, offset = INT_MAX; j < shm_cfg->SO_MERCI; j++) {
                if (shm_goods[i + j] < 0 && shm_goods_template[j].tons < offset) {
                    offset = shm_goods_template[j].tons;
                }
            }

            /* se il porto non ha richieste non ha senso iterare */
            if (offset == INT_MAX) continue;

            memset(tmp_goods_to_get, 0, sizeof(int) * shm_cfg->SO_MERCI);

            for(j = 0, ship_tons_available = shm_cfg->SO_CAPACITY; j < shm_cfg->SO_MERCI && ship_tons_available > offset; j++) {
                if(shm_goods[id + j] > 0 && shm_goods[i + j] < 0 && ship_tons_available / shm_goods_template[j].tons) {
                    /* quantità totale che posso caricare in nave */
                    min_val = shm_goods[id + j] < -shm_goods[i + j] ? shm_goods[id + j] : -shm_goods[i + j];

                    for (; min_val && (min_val * shm_goods_template[j].tons) > ship_tons_available; min_val--);

                    if (!min_val) {
                        continue;
                    }

                    tmp_goods_to_get[j] = min_val;
                    ship_tons_available -= min_val * shm_goods_template[j].tons;
                }
            }

            if (ship_tons_available < best_tons_available) {
                best_route = i;
                best_tons_available = ship_tons_available;
                memcpy(goods_to_get, tmp_goods_to_get, sizeof(int) * shm_cfg->SO_MERCI);
            }
        }
    }

    if(best_tons_available < shm_cfg->SO_CAPACITY) {
        for (i = 0; i < shm_cfg->SO_MERCI; i++) {
            /* print and update doesn't work due to race problems  obviously they'll not be implemented like this*/
            if (goods_to_get[i]) {
                printf("[%d] Merch id [%d] requested: %d, tons exchanged: %d\n", getpid(), i,
                       goods_to_get[i], (goods_to_get[i] * shm_goods_template[i].tons));
                /* shm_goods[id + i] -= goods_to_get[i]; */
                printf("[%d] Pre-update: quantity offer [%d], quantity request [%d]\n", getpid(), shm_goods[id + i], shm_goods[best_route + i]);
                __sync_fetch_and_sub(&shm_goods[id + i], goods_to_get[i]);
                shm_dump_ports[id].good_send += goods_to_get[i];
                if (shm_goods[id + i] < 0)
                    printf("\033[31;1m[%d] Qualocsa è andato storto con la merce offerta: %d\033[;0m\n", getpid(), shm_goods[id + i]);
                __sync_fetch_and_add(&shm_goods[best_route + i], goods_to_get[i]);
                shm_dump_ports[best_route].good_received += goods_to_get[i];
                printf("[%d] Post-update: quantity offer [%d], quantity request [%d]\n", getpid(), shm_goods[id + i], shm_goods[best_route + i]);
                /* shm_goods[best_route + i] += goods_to_get[i]; */
                 if (shm_goods[best_route + i] > 0)
                     printf("\033[31;1m[%d] Qualocsa è andato storto con la merce richiesta: %d\033[;0m\n", getpid(), shm_goods[best_route + i]);
            }
        }
        printf("Route found: from %d to %d with %d tons available to exchange\n", id, best_route, (shm_cfg->SO_CAPACITY - best_tons_available));
    } else best_route = -1;

    free(goods_to_get);
    free(tmp_goods_to_get);

    return best_route;
}

void porto_sig_handler(int signum) {
    int old_errno = errno;
    int i;

    switch (signum) {
        case SIGTERM:
            exit(EXIT_SUCCESS);
        case SIGCONT:
            if (shm_cfg->GENERATING_PORTS > 0) {
                ton_in_excess = generate_goods(0);
            }
            break;
        case SIGINT:
            exit(EXIT_FAILURE);
        case SIGALRM:
            /*TODO: dump stato attuale*/
            /*printf("[PORTO] DUMP PID: [%d] SIGALRM\n", getpid());*/
            for (i = 0, shm_dump_ports[id].good_available = 0; i < shm_cfg->SO_MERCI; i++) {
                if (shm_goods[id + i] > 0) {
                    shm_dump_ports[id].good_available += shm_goods[id + i];
                }
            }
            shm_dump_ports[id].id = id;
            shm_dump_ports[id].dock_available = semctl(shm_cfg->sem_id_dock, id, GETVAL);

            if (shm_pid_array[id] < 0 && shm_cfg->GENERATING_PORTS > 0) {
                ton_in_excess = generate_goods(ton_in_excess);
                /* printf("[%d] PORT_ID[%d] GOT SIGURS2 SIGNAL, GENERATING GOODS\n", getpid(), id); */
                shm_pid_array[id] = -shm_pid_array[id];
            }
            while (sem_cmd(shm_cfg->sem_id_gen_precedence, 0, -1, 0)) {
                CHECK_ERROR_CHILD(errno != EINTR,
                                  "[PORTO] Error while trying to release sem_id_gen_precedence")
            }
            break;
        case SIGUSR1:
            /* swell occurred */
            printf("[PORTO] SWELL: %d\n", getpid());
            shm_dump_ports[id].on_swell = 1;
            nanosleep_function(shm_cfg->SO_SWELL_DURATION / 24.0 * shm_cfg->SO_DAY_LENGTH,
                               "Generic error while sleeping because of the swell");
            shm_dump_ports[id].on_swell = 0;
            printf("[PORTO] END SWELL: %d\n", getpid());
            break;
        case SIGUSR2:
            break;
        default:
            printf("[PORTO] Signal: %s\n", strsignal(signum));
            break;
    }

    errno = old_errno;
}