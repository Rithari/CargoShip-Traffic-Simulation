#include "../headers/master.h"
#include "../headers/utils.h"
#include "../headers/common_ipcs.h"
#include <math.h>
#include <float.h>

/*se una nave non ha banchine e code di attracco libere allora viene gettata in mare*/
/*nave deve ricordarsi l'ultimo porto di partenza per evitare che ci ritorni quando viene messa in mare */
/*una nave in mare cerca sempre una coda di attracco libera (BUSY WAITING!!)*/

/*TODO: GUI!*/

void move(int);
int get_nearest_port_from_sea(void);
void nave_sig_handler(int);

config  *shm_cfg;
generalGoods *shm_goods_template;
coord   *shm_ports_coords;
dump_ships  *shm_dump_ships;
dump_goods  *shm_dump_goods;

int     shm_id_config;

coord   actual_coordinate;
int     actual_capacity;
int     id_actual_port;
int     id_destination_port;


/* TODO: Attach to message queues */
int main(int argc, char** argv) {
    int i;
    double rndx;
    double rndy;

    struct sigaction sa;
    struct sembuf sops;

    if(argc != 2) {
        printf("Incorrect number of parameters [%d]. Exiting...\n", argc);
        kill(getppid(), SIGINT);
    }

    srandom(getpid());

    shm_id_config = string_to_int(argv[1]);
    CHECK_ERROR_CHILD(errno, "[NAVE] Error while trying to convert shm_id_config")

    CHECK_ERROR_CHILD((shm_cfg = shmat(shm_id_config, NULL, SHM_RDONLY)) == (void*) -1,
                      "[NAVE] Error while trying to attach to configuration shared memory")
    CHECK_ERROR_CHILD((shm_ports_coords = shmat(shm_cfg->shm_id_ports_coords, NULL, SHM_RDONLY)) == (void*) -1,
                      "[NAVE] Error while trying to attach to ports coordinates shared memory")
    CHECK_ERROR_CHILD((shm_goods_template = shmat(shm_cfg->shm_id_goods_template, NULL, SHM_RDONLY)) == (void*) -1,
                      "[NAVE] Error while trying to attach to goods template shared memory")
    CHECK_ERROR_CHILD((shm_dump_ships = shmat(shm_cfg->shm_id_dump_ships, NULL, 0)) == (void*) -1,
                      "[NAVE] Error while trying to attach to dump ships shared memory")
    CHECK_ERROR_CHILD((shm_dump_goods = shmat(shm_cfg->shm_id_dump_goods, NULL, 0)) == (void*) -1,
                      "[NAVE] Error while trying to attach to dump goods shared memory")

    id_actual_port = -1;

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
    id_destination_port = -1; /* ship in sea */

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = nave_sig_handler;
    sa.sa_flags = SA_RESTART;
    sigaction(SIGCONT, &sa, NULL);
    sigaction(SIGALRM, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sa.sa_flags |= SA_NODEFER;
    sigaction(SIGUSR1, &sa, NULL);

    CHECK_ERROR_CHILD(sem_cmd(shm_cfg->sem_id_gen_precedence, 0, -1, 0) < 0,
                      "[NAVE] Error while trying to release sem_id_gen_precedence")

    /* Wait until everyone is ready (master will send SIGCONT) */
    pause();

    /* First iteration, ship on sea */
    id_destination_port = get_nearest_port_from_sea();
    printf("[%d] Chose port no: [%d] from [%d]\n", getpid(), id_destination_port, id_actual_port);
    sops.sem_num = id_destination_port;
    sops.sem_op = -1;
    sops.sem_flg = 0;

    move(id_destination_port);

    while (semop(shm_cfg->sem_id_dock, &sops, 1)) {
        CHECK_ERROR_CHILD(errno != EINTR, "[NAVE] Error while locking sem_id_dock[id_destination_port] semaphore")
    }

    id_actual_port = id_destination_port;

    /* now ship have a dock */
    while (1) {
        /* TODO: Pick a destination port based on the best request the ship can fulfill */
        /* scelta della tratta, stablita la tratta procedo a chiedere la banchina */
        /* nella versione definitiva sarà il porto a definire la tratta */
        do {
            id_destination_port = (int) random() % shm_cfg->SO_PORTI;
        } while (id_destination_port == id_actual_port);

        /* Per adesso mi limito a scegliere un porto casuale e richiedere l'accesso alla banchina
         * printf("[%d] Choose port no: [%d] from [%d]\n", getpid(), id_destination_port, id_actual_port);*/


        sops.sem_num = id_actual_port;
        sops.sem_op =  1;
        sops.sem_flg = 0;

        /* TODO: settare il semaforo per il successo dell'operazione a zero, ovvero non ho più una banchina */
        while (semop(shm_cfg->sem_id_dock, &sops, 1)) {
            CHECK_ERROR_CHILD(errno != EINTR, "[NAVE] Error while freeing sem_id_dock[id_actual_port] semaphore")
        }
        id_actual_port = -1;

        sops.sem_num = id_destination_port;
        sops.sem_op = -1;
        sops.sem_flg = 0;

        move(id_destination_port);

        /*TODO: errore mentre una nave muore!!*/
        /*TODO: e se utilizzassi un altro semaforo per indicare il successo dell'operazione?
                Sostanzialmente lo utilizzo come un flag, se attivo ho il lock altrimenti no
                utile in sigterm sicuramente */
        while (semop(shm_cfg->sem_id_dock, &sops, 1)) {
            CHECK_ERROR_CHILD(errno != EINTR, "[NAVE] Error while locking sem_id_dock[id_destination_port] semaphore")
        }

        id_actual_port = id_destination_port;

        /* Ship got a dock, now we can do some operation */
    }
}

void move(int id_destination) {
    struct timespec ts, rem;

    double dx = shm_ports_coords[id_destination].x - actual_coordinate.x;
    double dy = shm_ports_coords[id_destination].y - actual_coordinate.y;
    /*distance / SO_SPEED*/
    double navigation_time = sqrt(dx * dx + dy * dy) / shm_cfg->SO_SPEED;

    ts.tv_sec = (long) navigation_time;
    ts.tv_nsec = (long) ((navigation_time - ts.tv_sec) * 1000000000);
    while (nanosleep(&ts, &rem)) {
        switch (errno) {
            case EINTR:
                /* TODO: aggiungere funzionalità (forse non necessario)*/
                ts = rem;
                /*printf("[%d] Interrupt occurred while travelling for port no: [%d], time left [s:  %ld\tns:    %ld]\n",
                       getpid(), id_destination, ts.tv_sec, ts.tv_nsec);*/
                continue;
            default:
                perror("[NAVE] Generic error in move");
                exit(EXIT_FAILURE);
        }
    }
    printf("[%d] Arrived in port no: [%d]\tNavigation time: %f\n", getpid(), id_destination, navigation_time);
    fflush(stdout);
    actual_coordinate = shm_ports_coords[id_destination];
}

/*TODO: useless function? */
int get_nearest_port_from_sea(void) {
    int port_index;
    int i;
    double distance = DBL_MAX;

    for(i = 0; i < shm_cfg->SO_PORTI; i++) {
        double dx = shm_ports_coords[i].x - actual_coordinate.x;
        double dy = shm_ports_coords[i].y - actual_coordinate.y;
        double d_tmp = dx * dx + dy * dy;
        if(d_tmp < distance) {
            port_index = i;
            distance = d_tmp;
        }
    }

    /* Return the index of the port we selected */
    return port_index;
}

void nave_sig_handler(int signum) {
    int old_errno = errno;
    struct timespec storm_duration, rem;

    switch (signum) {
        case SIGCONT:
            break;
        case SIGALRM:
            /*TODO: dump stato attuale*/
            /* printf("[NAVE] DUMP PID: [%d] SIGALRM\n", getpid()); */
            CHECK_ERROR_CHILD(sem_cmd(shm_cfg->sem_id_gen_precedence, 0, -1, 0) < 0,
                              "[NAVE] Error while trying to release sem_id_gen_precedence")
            break;
        case SIGTERM:
            /* malestorm killed the ship :C or program end*/
            /* mascherare altri segnali ?*/
            /* doesn't work... */
            if(id_actual_port >= 0) {
                CHECK_ERROR_CHILD(sem_cmd(shm_cfg->sem_id_dock, id_actual_port, 1, 0),
                                  "[NAVE] Unable to free the id_destination_port dock")
            }
            if (sem_cmd(shm_cfg->sem_id_gen_precedence, 0, -1, IPC_NOWAIT) < 0) {
                CHECK_ERROR_CHILD(errno != EAGAIN, "[PORTO] Error while trying to release sem_id_gen_precedence")
            }
            exit(EXIT_SUCCESS);
        case SIGUSR1:
            /* storm occurred */
            printf("[NAVE] STORM: %d\n", getpid());

            storm_duration = calculate_timeout(shm_cfg->SO_STORM_DURATION, shm_cfg->SO_DAY_LENGTH);

            while (nanosleep(&storm_duration, &rem)) {
                switch (errno) {
                    case EINTR:
                        storm_duration = rem;
                        continue;
                    default:
                        perror("[NAVE] Generic error while sleeping");
                        exit(EXIT_FAILURE);
                }
            }
            break;
        default:
            printf("[NAVE] Signal: %s\n", strsignal(signum));
            break;
    }

    errno = old_errno;
}
