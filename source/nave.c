#include "../headers/master.h"
#include "../headers/utils.h"
#include "../headers/common_ipcs.h"
#include <math.h>

/*se una nave non ha banchine e code di attracco libere allora viene gettata in mare*/
/*nave deve ricordarsi l'ultimo porto di partenza per evitare che ci ritorni quando viene messa in mare */
/*una nave in mare cerca sempre una coda di attracco libera (BUSY WAITING!!)*/

void move(int);
int pick_rand_port_on_sea(void);
void nave_sig_handler(int);

config  *shm_cfg;
coord   *shm_ports_coords;
int     shm_id_config;
int     shm_id_ports_coords;
int     mq_id_request;
int     sem_id_generation;
int     sem_id_docks;

coord   actual_coordinate;
int     actual_capacity;
int     old_id_destination_port;
int     id_destination_port;

enum {false, true};
typedef int bool;


/* TODO: Attach to message queues */
int main(int argc, char** argv) {
    int i;
    double rndx;
    double rndy;

    struct sigaction sa;
    struct sembuf sops;

    if(argc != 6) {
        printf("Incorrect number of parameters [%d]. Exiting...\n", argc);
        kill(getppid(), SIGINT);
    }

    srandom(getpid());

    /* TODO: CONTROLLARE IL FALLIMENTO DI QUESTA SEZIONE DI CODICE UNA VOLTA FATTO IL REFACTORING */
    shm_id_config = string_to_int(argv[1]);
    shm_id_ports_coords = string_to_int(argv[2]);
    mq_id_request = string_to_int(argv[3]);
    sem_id_generation = string_to_int(argv[4]);
    sem_id_docks = string_to_int(argv[5]);


    if((shm_cfg = shmat(shm_id_config, NULL, SHM_RDONLY)) == (void*) -1) {
        perror("[NAVE] Error while trying to attach to configuration shared memory");
        kill(getppid(), SIGINT);
    }

    if((shm_ports_coords = shmat(shm_id_ports_coords, NULL, SHM_RDONLY)) == (void*) -1) {
        perror("[NAVE] Error while trying to attach to ports coordinates shared memory");
        kill(getppid(), SIGINT);
    }

    old_id_destination_port = -1;

    rndx = (double) random() / RAND_MAX * shm_cfg->SO_LATO;
    rndy = (double) random() / RAND_MAX * shm_cfg->SO_LATO;

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
    sigaction(SIGALRM, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sa.sa_flags |= SA_NODEFER;
    sigaction(SIGUSR1, &sa, NULL);

    if(sem_cmd(sem_id_generation, 0, -1, 0) < 0) {
        perror("[NAVE] Error while trying to release sem_id_generation");
        kill(getppid(), SIGINT);
    }

    pause();

    id_destination_port = pick_rand_port_on_sea();
    printf("[%d] Choose port no: [%d] from [%d]\n", getpid(), id_destination_port, old_id_destination_port);
    move(id_destination_port);

    while (1) {
        old_id_destination_port = id_destination_port;

        /* scelta della tratta, stablita la tratta procedo a chiedere la banchina */
        /* nella versione definitiva sarà il porto a definire la tratta */
        do {
            id_destination_port = (int) random() % shm_cfg->SO_PORTI;
        } while (id_destination_port == old_id_destination_port);

        /* Per adesso mi limito a scegliere un porto casuale e richedere l'accesso alla banchina */
        printf("[%d] Choose port no: [%d] from [%d]\n", getpid(), id_destination_port, old_id_destination_port);

        sops.sem_num = old_id_destination_port;
        sops.sem_op =  1;
        sops.sem_flg = 0;

        while (semop(sem_id_docks, &sops, 1)) {
            switch (errno) {
                case EINTR:
                    continue;
                default:
                    perror("[NAVE] Error in pick_rand_port()");
                    kill(getppid(), SIGINT);
                    break;
            }
        }

        sops.sem_num = id_destination_port;
        sops.sem_op = -1;
        sops.sem_flg = 0;

        move(id_destination_port);

        while (semop(sem_id_docks, &sops, 1)) {
            switch (errno) {
                case EINTR:
                    continue;
                default:
                    perror("[NAVE] Error in pick_rand_port()");
                    kill(getppid(), SIGINT);
                    break;
            }
        }
    }
}

void nave_sig_handler(int signum) {
    int old_errno = errno;
    struct timespec storm_duration, rem;

    switch (signum) {
        case SIGALRM:
            /*printf("Allarme!\n"); */
            /*TODO: dump stato attuale*/
            break;
        case SIGTERM:
            /* malestorm killed the ship :C */
            /* mascherare altri segnali */
            /*TODO: dump stato attuale*/
            if(id_destination_port >= 0) {
                CHECK_ERROR(sem_cmd(sem_id_docks, id_destination_port, 1, 0), getppid(),
                            "[NAVE] Unable to free the \"id_destination_port\" dock")
            }
            exit(EXIT_SUCCESS);
        case SIGUSR1:
            /* storm occurred */
            printf("[PORTO] SWELL: %d\n", getpid());

            storm_duration = calculate_timeout(shm_cfg->SO_STORM_DURATION, shm_cfg->SO_DAY_LENGTH);

            while (nanosleep(&storm_duration, &rem)) {
                switch (errno) {
                    case EINTR:
                        storm_duration = rem;
                        continue;
                    default:
                        perror("[PORTO] Generic error while sleeping");
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
    /* struct timespec start, end; */

    double dx = shm_ports_coords[id_destination].x - actual_coordinate.x;
    double dy = shm_ports_coords[id_destination].y - actual_coordinate.y;

    /*distance / SO_SPEED*/
    double navigation_time = sqrt(dx * dx + dy * dy) / shm_cfg->SO_SPEED;

    ts.tv_sec = (long) navigation_time;
    ts.tv_nsec = (long) ((navigation_time - ts.tv_sec) * 1000000000);
    printf("I'm moving");
    while (nanosleep(&ts, &rem)) {
        switch (errno) {
            case EINTR:
                /* TODO: aggiungere funzionalità (forse non necessario)*/
                /*
                 *  clock_gettime(CLOCK_REALTIME, &start);
                    clock_gettime(CLOCK_REALTIME, &end);
                    timespec_sub(&end, &end, &start);
                    timespec_sub(&rem, &rem, &end);
                 * */
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

int pick_rand_port_on_sea(void) {
    /* Keep trying to pick a random port and acquire a lock on its dock until we succeed */
    /*TODO: evitare che si possa scegliere il porto di provenienza*/
    int port_index = (int) random() % shm_cfg->SO_PORTI;

    /* Select a random port from the coordinates array */
    /* Try to acquire a lock on the port's semaphore */
    /* Decrement the port's semaphore value to acquire a lock on the dock */
    while (sem_cmd(sem_id_docks, port_index, -1, 0)) {
        /* The port's semaphore is not available (its value is 0) */
        switch(errno) {
            case EINTR:
            case EAGAIN:
                /* Select a different port and try again */
                /*TODO: evitare che si possa scegliere il porto di provenienza*/
                port_index = (port_index + 1) % shm_cfg->SO_PORTI;
                continue;
            default:
                /* Generic error */
                perror("[NAVE] Error in pick_rand_port()");
                kill(getppid(), SIGINT);
                break;
        }
    }

    /* Return the index of the port we selected */
    return port_index;
}
