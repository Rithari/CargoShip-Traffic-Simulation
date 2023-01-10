#include "../headers/master.h"
#include "../headers/utils.h"
#include "../headers/common_ipcs.h"
#include "../headers/linked_list.h"
#include <math.h>

/*se una nave non ha banchine e code di attracco libere allora viene gettata in mare*/
/*nave deve ricordarsi l'ultimo porto di partenza per evitare che ci ritorni quando viene messa in mare */
/*una nave in mare cerca sempre una coda di attracco libera (BUSY WAITING!!)*/

/*TODO: GUI!*/

void move(int);
int get_nearest_port(void);
void nave_sig_handler(int);

config  *shm_cfg;
goods_template   *shm_goods_template;
coord   *shm_ports_coords;
int   *shm_goods;
pid_t   *shm_pid_array;
dump_ships  *shm_dump_ships;
dump_goods  *shm_dump_goods;

int     shm_id_config;
int     id;

coord   actual_coordinate;
unsigned int     actual_capacity;
int     id_actual_port;
int     id_destination_port;
struct node *head;


/* TODO: Attach to message queues */
int main(int argc, char** argv) {
    int i;
    int selected_good = 0;
    double rndx;
    double rndy;
    double time_to_sleep = 0;

    struct sigaction sa;
    struct sembuf sops;
    msg_handshake msg;
    msg_goods msg_g;
    struct node *cur;

    if(argc != 3) {
        printf("Incorrect number of parameters [%d]. Exiting...\n", argc);
        kill(getppid(), SIGINT);
    }

    srandom(getpid());
    head = NULL;

    shm_id_config = string_to_int(argv[1]);
    CHECK_ERROR_CHILD(errno, "[NAVE] Error while trying to convert shm_id_config")
    id = string_to_int(argv[2]);
    CHECK_ERROR_CHILD(errno, "[NAVE] Error while trying to convert shm_id_config")

    CHECK_ERROR_CHILD((shm_cfg = shmat(shm_id_config, NULL, SHM_RDONLY)) == (void*) -1,
                      "[NAVE] Error while trying to attach to configuration shared memory")
    CHECK_ERROR_CHILD((shm_ports_coords = shmat(shm_cfg->shm_id_ports_coords, NULL, SHM_RDONLY)) == (void*) -1,
                      "[NAVE] Error while trying to attach to ports coordinates shared memory")
    CHECK_ERROR_CHILD((shm_goods = shmat(shm_cfg->shm_id_goods, NULL, 0)) == (void*) -1,
                      "[NAVE] Error while trying to attach to goods shared memory")
    CHECK_ERROR_CHILD((shm_pid_array = shmat(shm_cfg->shm_id_pid_array, NULL, 0)) == (void*) -1,
                      "[NAVE] Error while trying to attach to pid_array shared memory")
    CHECK_ERROR_CHILD((shm_goods_template = shmat(shm_cfg->shm_id_goods_template, NULL, SHM_RDONLY)) == (void*) -1,
                      "[NAVE] Error while trying to attach to goods_template shared memory")
    CHECK_ERROR_CHILD((shm_dump_ships = shmat(shm_cfg->shm_id_dump_ships, NULL, 0)) == (void*) -1,
                      "[NAVE] Error while trying to attach to dump ships shared memory")
    CHECK_ERROR_CHILD((shm_dump_goods = shmat(shm_cfg->shm_id_dump_goods, NULL, 0)) == (void*) -1,
                      "[NAVE] Error while trying to attach to dump goods_template shared memory")

    rndx = (double) random() / RAND_MAX * shm_cfg->SO_LATO;
    rndy = (double) random() / RAND_MAX * shm_cfg->SO_LATO;

    /* Avoid placing the ship at the same coordinates of a port */
    for(i = 0; i < shm_cfg->SO_PORTI; i++) {
        if(shm_ports_coords[i].x == rndx && shm_ports_coords[i].y == rndy) {
            i = -1;
            rndx = (double) random() / RAND_MAX * shm_cfg->SO_LATO;
            rndy = (double) random() / RAND_MAX * shm_cfg->SO_LATO;
        }
    }

    actual_coordinate.x = rndx;
    actual_coordinate.y = rndy;
    actual_capacity = 0;
    id_actual_port = -1;
    id_destination_port = -1; /* ship in sea */

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = nave_sig_handler;
    sigaction(SIGCONT, &sa, NULL);
    sigaction(SIGUSR2, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sa.sa_flags |= SA_NODEFER;
    sigaction(SIGUSR1, &sa, NULL);
    sigaddset(&sa.sa_mask, SIGUSR1);
    sigaddset(&sa.sa_mask, SIGTERM);
    sigaction(SIGALRM, &sa, NULL);


    CHECK_ERROR_CHILD(sem_cmd(shm_cfg->sem_id_gen_precedence, 0, -1, 0) < 0,
                      "[NAVE] Error while trying to release sem_id_gen_precedence")

    /* Wait until everyone is ready (master will send SIGCONT) */
    pause();

    /* now ship have a dock */
    while (1) {
        /* TODO: Pick a destination port based on the best request the ship can fulfill */
        /* scelta della tratta, stablita la tratta procedo a chiedere la banchina */
        /* nella versione definitiva sarà il porto a definire la tratta */
        if(id_actual_port == -1) {
            id_destination_port = get_nearest_port();
        } else {
            sops.sem_num = id_actual_port;
            sops.sem_op =  1;
            sops.sem_flg = SEM_UNDO;

            while (semop(shm_cfg->sem_id_dock, &sops, 1)) {
                CHECK_ERROR_CHILD(errno != EINTR, "[NAVE] Error while freeing sem_id_dock[id_actual_port] semaphore")
            }
            id_actual_port = -1;
        }

        /* se si volesse implementare la possibilità che id_Destination_port sia == a -1 (nave va in mare)
         * basta fare un controllo da qui fino alla fine*/
        sops.sem_num = id_destination_port;
        sops.sem_op = -1;
        sops.sem_flg = SEM_UNDO;

        move(id_destination_port);

        while (semop(shm_cfg->sem_id_dock, &sops, 1)) {
            CHECK_ERROR_CHILD(errno != EINTR, "[NAVE] Error while locking sem_id_dock[id_destination_port] semaphore")
        }
        id_actual_port = id_destination_port;
        /* Getting permission to load/unload */
        /* devo ricevere l'id della mq per le comunicazioni e la quantità di merce da leggere*/


        /* unloading goods */
        while (head) {
            nanosleep_function((double) head->element->quantity * shm_goods_template[head->element->id].tons * shm_cfg->SO_DAY_LENGTH / shm_cfg->SO_LOADSPEED,
                               "[NAVE] Generic error while unloading the ship");
            /* update dumps */
            free(head->element);
            cur = head;
            head = head->next;
            free(cur);
        }

        printf("[%d] Unload operation done!\n", getpid());

        msg.mtype = id_actual_port + 1;
        msg.response_pid = getpid();
        while (msgsnd(shm_cfg->mq_id_ports_handshake, &msg, sizeof(msg) - sizeof(long), 0)) {
            CHECK_ERROR_CHILD(errno != EINTR, "[NAVE] Error while sending handshake message")
        }
        while (msgrcv(shm_cfg->mq_id_ships_handshake, &msg, sizeof(msg) - sizeof(long), getpid(), 0) < 0) {
            CHECK_ERROR_CHILD(errno != EINTR, "[NAVE] Error while waiting handshake message")
        }

        printf("SIUM 2\n");

        if (msg.response_pid >= 0) {
            printf("[%d] Sto andando al porto: %d e sto caricando: %d merci\n", getpid(), msg.response_pid, msg.how_many);
            for (i = 0; i < msg.how_many; i++) {
                while (msgrcv(shm_cfg->mq_id_ships_goods, &msg_g, sizeof(msg_goods) - sizeof(long), getpid(), 0) < 0) {
                    CHECK_ERROR_CHILD(errno != EINTR, "[NAVE] Error while waiting good message")
                }
                printf("[%d] [%d/%d/%d]\n ", getpid(), msg_g.to_add.id, msg_g.to_add.quantity, msg_g.to_add.lifespan);
                head = ll_add(head, &msg_g.to_add);
                ll_print(head);
                time_to_sleep += msg_g.to_add.quantity * shm_goods_template[msg_g.to_add.id].tons;
            }

            nanosleep_function(time_to_sleep * shm_cfg->SO_DAY_LENGTH / shm_cfg->SO_LOADSPEED,
                               "[NAVE] Generic error while loading the ship");

            shm_dump_goods[selected_good].state++;

            printf("[%d] Load operation!\n", getpid());
            /* destinazione delle merci se esiste una tratta*/
            id_destination_port = msg.response_pid;

            printf("[%d] Choose port no: [%d] from [%d]\n", getpid(), id_destination_port, id_actual_port);
        } else {
            /* destinazione delle merci se non esiste una tratta*/
            id_destination_port = get_nearest_port();
            printf("[%d] No route found, ship will go in port [%d] :C\n", getpid(), id_destination_port);
        }
    }
}

void move(int id_destination) {
    double dx = shm_ports_coords[id_destination].x - actual_coordinate.x;
    double dy = shm_ports_coords[id_destination].y - actual_coordinate.y;
    double navigation_time = sqrt(dx * dx + dy * dy) / shm_cfg->SO_SPEED * shm_cfg->SO_DAY_LENGTH;

    shm_pid_array[id + shm_cfg->SO_PORTI] = -shm_pid_array[id + shm_cfg->SO_PORTI];
    nanosleep_function(navigation_time, "[NAVE] Generic error while moving");
    shm_pid_array[id + shm_cfg->SO_PORTI] = -shm_pid_array[id + shm_cfg->SO_PORTI];

    printf("[%d] Arrived in port no: [%d]\tNavigation time: %f\n", getpid(), id_destination, navigation_time);
    fflush(stdout);
    actual_coordinate = shm_ports_coords[id_destination];
}


int get_nearest_port(void) {
    int i, j, k;

    for(k = 0, i = (int) random() % shm_cfg->SO_PORTI; k < shm_cfg->SO_PORTI; k++, i = (i + 1) % shm_cfg->SO_PORTI) {
        if (i == id_actual_port) continue;

        for (j = 0; j < shm_cfg->SO_MERCI; j++) {
            if (shm_goods[i * shm_cfg->SO_MERCI + j] > 0 && shm_cfg->SO_CAPACITY / shm_goods_template[j].tons) {
                return i;
            }
        }
    }

    do {
        i = (int) random() % shm_cfg->SO_PORTI;
    } while (i == id_actual_port);

    /* Return a random port, no one is good... */
    /* TODO: possibilitò di tornare al porto di partenza :C */
    return i;
}

/*int get_nearest_port(void) {
    /* TODO: ABSOLUTE GARBAGE!!!!!!!! *
    int i, j, k;
    int port_index = -1;
    double distance = DBL_MAX;

    for(k = 0, i = (int) random() % shm_cfg->SO_PORTI; k < shm_cfg->SO_PORTI; k++, i = (i + 1) % shm_cfg->SO_PORTI) {
        if (i == id_actual_port) continue;

        for (j = 0; j < shm_cfg->SO_MERCI; j++) {
            if (shm_goods[i * shm_cfg->SO_MERCI + j] > 0 && shm_cfg->SO_CAPACITY / shm_goods_template[j].tons) {
                double dx = shm_ports_coords[i].x - actual_coordinate.x;
                double dy = shm_ports_coords[i].y - actual_coordinate.y;
                double d_tmp = dx * dx + dy * dy;

                if(d_tmp < distance) {
                    port_index = i;
                    distance = d_tmp;
                }
                break;
            }
        }
    }

    /* Return a random port, no one is good... *
    /* TODO: possibilitò di tornare al porto di partenza :C *
    return port_index > 0 ? port_index : (int) random() % shm_cfg->SO_PORTI;
}*/

void dump_ship_data(void) {
    /* sem_id_dump_mutex[0] is a mutex semaphore utilized for dump ship's cargo*/
    /*TODO: dump stato attuale*/
    while (sem_cmd(shm_cfg->sem_id_dump_mutex, 0, -1, 0)) {
        CHECK_ERROR_CHILD(errno != EINTR, "[NAVE] Error while trying to get sem_id_dump_mutex[0]")
    }
    if (id_actual_port < 0) {
        if (actual_capacity) {
            shm_dump_ships->with_cargo_en_route++;
        } else {
            shm_dump_ships->without_cargo_en_route++;
        }
    } else {
        shm_dump_ships->being_loaded_unloaded++;
    }
    while (sem_cmd(shm_cfg->sem_id_dump_mutex, 0, 1, 0)) {
        CHECK_ERROR_CHILD(errno != EINTR, "[NAVE] Error while trying to release sem_id_dump_mutex[0]")
    }
}

void nave_sig_handler(int signum) {
    int old_errno = errno;

    switch (signum) {
        case SIGCONT:
            break;
        case SIGTERM:
            dump_ship_data();
            if (id_actual_port >= 0) {
                /*
                 * kill(abs(shm_pid_array[id_actual_port]), SIGUSR2);
                 * */
            }
            exit(EXIT_SUCCESS);
        case SIGALRM:
            dump_ship_data();
            while (sem_cmd(shm_cfg->sem_id_gen_precedence, 0, -1, 0)) {
                CHECK_ERROR_CHILD(errno != EINTR,
                                  "[NAVE] Error while trying to release sem_id_gen_precedence")
            }
            break;
        case SIGUSR1:
            /* storm occurred */
            printf("[NAVE] STORM: %d\n", getpid());
            while (sem_cmd(shm_cfg->sem_id_dump_mutex, 1, -1, 0)) {
                CHECK_ERROR_CHILD(errno != EINTR, "[NAVE] Error while trying to release sem_id_dump_mutex[0]")
            }
            shm_dump_ships->slowed++;
            while (sem_cmd(shm_cfg->sem_id_dump_mutex, 1, 1, 0)) {
                CHECK_ERROR_CHILD(errno != EINTR, "[NAVE] Error while trying to release sem_id_dump_mutex[0]")
            }
            nanosleep_function(shm_cfg->SO_STORM_DURATION / 24.0 * shm_cfg->SO_DAY_LENGTH,
                               "[NAVE] Generic error while sleeping because of the storm");
            break;
        case SIGUSR2:
            /* malestorm killed the ship :C*/
            /* shm_dump_ships->sunk++; */
            /* TODO: rimuovere tutti i __sync_fetch_and_xxx */
            __sync_fetch_and_add(&shm_dump_ships->sunk, 1);
            /* nave affondata, aggiornare i dump della merce persa */
            /* kill(abs(shm_pid_array[id_actual_port]), SIGUSR2); */
            if (sem_cmd(shm_cfg->sem_id_gen_precedence, 0, -1, IPC_NOWAIT)) {
                CHECK_ERROR_CHILD(errno != EINTR && errno != EAGAIN,
                                  "[NAVE] Error while trying to release sem_id_gen_precedence")
            }
            exit(EXIT_SUCCESS);
        default:
            printf("[NAVE] Signal: %s\n", strsignal(signum));
            break;
    }

    errno = old_errno;
}
