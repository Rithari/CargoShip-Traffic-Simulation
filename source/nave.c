#include "../headers/master.h"
#include "../headers/utils.h"
#include "../headers/common_ipcs.h"
#include <math.h>

/*se una nave non ha banchine e code di attracco libere allora viene gettata in mare*/
/*nave deve ricordarsi l'ultimo porto di partenza per evitare che ci ritorni quando viene messa in mare */
/*una nave in mare cerca sempre una coda di attracco libera (BUSY WAITING!!)*/

/*TODO: GUI!*/

void move(int);
int pick_rand_port_on_sea(void);
void nave_sig_handler(int);

config  *shm_cfg;
goods   *shm_goods_template;
coord   *shm_ports_coords;

int     shm_id_config;
int     shm_id_ports_coords;
int     shm_id_goods_template;
int     mq_id_request;
int     sem_id_gen_precedence;
int     sem_id_docks;

coord   actual_coordinate;
int     actual_capacity;
int     old_id_destination_port;
int     id_destination_port;


/* TODO: Attach to message queues */
int main(int argc, char** argv) {
    int i;
    double rndx;
    double rndy;

    struct sigaction sa;
    struct sembuf sops;

    if(argc != 7) {
        printf("Incorrect number of parameters [%d]. Exiting...\n", argc);
        kill(getppid(), SIGINT);
    }

    srandom(getpid());

    /* TODO: Refactor and comment this section of code same for line 61 in porto.c */
    shm_id_config = string_to_int(argv[1]);
    CHECK_ERROR(errno, getppid(), "[NAVE] Error while trying to convert shm_id_config")
    shm_id_ports_coords = string_to_int(argv[2]);
    CHECK_ERROR(errno, getppid(), "[NAVE] Error while trying to convert shm_id_ports_coords")
    shm_id_goods_template = string_to_int(argv[3]);
    CHECK_ERROR(errno, getppid(), "[NAVE] Error while trying to convert shm_id_goods_template")
    mq_id_request = string_to_int(argv[4]);
    CHECK_ERROR(errno, getppid(), "[NAVE] Error while trying to convert mq_id_request")
    sem_id_gen_precedence = string_to_int(argv[5]);
    CHECK_ERROR(errno, getppid(), "[NAVE] Error while trying to convert sem_id_precedence")
    sem_id_docks = string_to_int(argv[6]);
    CHECK_ERROR(errno, getppid(), "[NAVE] Error while trying to convert sem_id_docks")

    CHECK_ERROR((shm_cfg = shmat(shm_id_config, NULL, SHM_RDONLY)) == (void*) -1, getppid(),
                "[NAVE] Error while trying to attach to configuration shared memory")
    CHECK_ERROR((shm_ports_coords = shmat(shm_id_ports_coords, NULL, SHM_RDONLY)) == (void*) -1, getppid(),
                "[NAVE] Error while trying to attach to ports coordinates shared memory")
    CHECK_ERROR((shm_goods_template = shmat(shm_id_goods_template, NULL, SHM_RDONLY)) == (void*) -1, getppid(),
                "[NAVE] Error while trying to attach to goods template shared memory")

    old_id_destination_port = -1;

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

    if(sem_cmd(sem_id_gen_precedence, 0, -1, 0) < 0) {
        perror("[NAVE] Error while trying to release sem_id_gen_precedence");
        kill(getppid(), SIGINT);
    }


    /* Wait until everyone is ready (master will send SIGCONT) */
    pause();

    id_destination_port = pick_rand_port_on_sea();
    printf("[%d] Chose port no: [%d] from [%d]\n", getpid(), id_destination_port, old_id_destination_port);
    move(id_destination_port);

    while (1) {
        old_id_destination_port = id_destination_port;

        /* TODO: Pick a destination port based on the best request the ship can fulfill */
        /* scelta della tratta, stablita la tratta procedo a chiedere la banchina */
        /* nella versione definitiva sarà il porto a definire la tratta */
        do {
            id_destination_port = (int) random() % shm_cfg->SO_PORTI;
        } while (id_destination_port == old_id_destination_port);

        /* Per adesso mi limito a scegliere un porto casuale e richiedere l'accesso alla banchina */
        printf("[%d] Choose port no: [%d] from [%d]\n", getpid(), id_destination_port, old_id_destination_port);

        sops.sem_num = old_id_destination_port;
        sops.sem_op =  1;
        sops.sem_flg = 0;

        while (semop(sem_id_docks, &sops, 1)) {
            CHECK_ERROR(errno != EINTR, getppid(), "[NAVE] Error while freeing semaphore")
        }

        sops.sem_num = id_destination_port;
        sops.sem_op = -1;
        sops.sem_flg = 0;

        move(id_destination_port);


        /*TODO: errore mentre una nave muore!!*/
        while (semop(sem_id_docks, &sops, 1)) {
            CHECK_ERROR(errno != EINTR, getppid(), "[NAVE] Error while locking semaphore")
        }
    }
}

void nave_sig_handler(int signum) {
    int old_errno = errno;
    struct timespec storm_duration, rem;

    switch (signum) {
        case SIGCONT:
            break;
        case SIGALRM:
            /*printf("Allarme!\n"); */
            /*TODO: dump stato attuale*/
            printf("[NAVE] DUMP PID: [%d] SIGALRM\n", getpid());
            break;
        case SIGTERM:
            /* malestorm killed the ship :C or program end*/
            /* mascherare altri segnali ?*/
            if(id_destination_port >= 0) {
                CHECK_ERROR(sem_cmd(sem_id_docks, id_destination_port, 1, 0), getppid(),
                            "[NAVE] Unable to free the \"id_destination_port\" dock")
            }
            exit(EXIT_SUCCESS);
        case SIGUSR1:
            /* storm occurred */
            printf("[NAVE] SWELL: %d\n", getpid());

            storm_duration = calculate_timeout(shm_cfg->SO_STORM_DURATION, shm_cfg->SO_DAY_LENGTH);

            while (nanosleep(&storm_duration, &rem)) {
                switch (errno) {
                    case EINTR:
                        storm_duration = rem;
                        continue;
                    default:
                        perror("[NAVE] Generic error while sleeping");
                        kill(getppid(), SIGINT);
                }
            }
            break;
        default:
            printf("[NAVE] Signal: %s\n", strsignal(signum));
            break;
    }

    errno = old_errno;
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
                printf("[%d] Interrupt occurred while travelling for port no: [%d], time left [s:  %ld\tns:    %ld]\n",
                       getpid(), id_destination, ts.tv_sec, ts.tv_nsec);
                continue;
            default:
                perror("Generic error in nave.c");
                kill(getppid(), SIGINT);
        }
    }
    printf("[%d] Arrived in port no: [%d]\tNavigation time: %f\n", getpid(), id_destination, navigation_time);
    fflush(stdout);
    actual_coordinate = shm_ports_coords[id_destination];
}

/*TODO: useless function*/
int pick_rand_port_on_sea(void) {
    /* Keep trying to pick a random port and acquire a lock on its dock until we succeed */
    int port_index;

    /* TODO: choose if it's better random or nearest port */
    do {
        port_index = (int) random() % shm_cfg->SO_PORTI;
    } while (port_index == old_id_destination_port);


    /* Select a random port from the coordinates array */
    /* Try to acquire a lock on the port's semaphore */
    /* Decrement the port's semaphore value to acquire a lock on the dock */
    while (sem_cmd(sem_id_docks, port_index, -1, 0)) {
        /* The port's semaphore is not available (its value is 0) */
        CHECK_ERROR(errno != EINTR, getppid(), "[NAVE] Error in pick_rand_port()")
        port_index = (port_index + 1) % shm_cfg->SO_PORTI == old_id_destination_port
                     ? (port_index + 2) % shm_cfg->SO_PORTI :
                     (port_index + 1) % shm_cfg->SO_PORTI;
    }

    /* Return the index of the port we selected */
    return port_index;
}
