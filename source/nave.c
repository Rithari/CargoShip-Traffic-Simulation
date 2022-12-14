#include "../headers/master.h"
#include "../headers/utils.h"
#include "../headers/common_ipcs.h"
#include <math.h>

/*se una nave non ha banchine e code di attracco libere allora viene gettata in mare*/
/*nave deve ricordarsi l'ultimo porto di partenza per evitare che ci ritorni quando viene messa in mare */
/*una nave in mare cerca sempre una coda di attracco libera (BUSY WAITING!!)*/

void nave_sig_handler(int);

config  *shm_cfg;
coord   *shm_ports_coords;
int     shm_id_config;
int     shm_id_ports_coords;
int     mq_id_request;
int     sem_id_generation;

coord   actual_coordinate;
int     actual_capacity;

void move(coord destination) {
    struct timespec ts, rem;
    struct timespec start, end;

    double dx = destination.x - actual_coordinate.x;
    double dy = destination.y - actual_coordinate.y;

    /*distance / SO_SPEED*/
    double navigation_time = sqrt(dx * dx + dy * dy) / shm_cfg->SO_SPEED;

    ts.tv_sec = (long) navigation_time;
    ts.tv_nsec = (long) ((navigation_time - ts.tv_sec) * 1000000000);

    while (nanosleep(&ts, &rem)) {
        switch (errno) {
            case EINTR:
                /* TODO: aggiungere funzionalità */
                /*
                 *  clock_gettime(CLOCK_REALTIME, &start);
                    clock_gettime(CLOCK_REALTIME, &end);
                    timespec_sub(&end, &end, &start);
                    timespec_sub(&rem, &rem, &end);
                 * */
                ts = rem;
                printf("Moving form [%f, %f] to [%f, %f].\tInterrupt occurred, time left [s:  %ld\tns:    %ld]\n",
                       actual_coordinate.x, actual_coordinate.y, destination.x, destination.y, ts.tv_sec, ts.tv_nsec);
                continue;
            default:
                perror("Generic error in nave.c");
                kill(getppid(), SIGINT);
        }
    }
    printf("Moved form [%f, %f] to [%f, %f].\tNavigation time: %f\n",
           actual_coordinate.x, actual_coordinate.y, destination.x, destination.y, navigation_time);
    fflush(stdout);
    actual_coordinate = destination;
}

/* TODO: Attach to message queues */
int main(int argc, char** argv) {
    int old_id_destination_port;
    int id_destination_port;
    int i;
    double rndx;
    double rndy;

    struct sigaction sa;

    if(argc != 4) {
        printf("Incorrect number of parameters [%d]. Exiting...\n", argc);
        kill(getppid(), SIGINT);
    }

    srandom(getpid());

    /* TODO: CONTROLLARE IL FALLIMENTO DI QUESTA SEZIONE DI CODICE UNA VOLTA FATTO IL REFACTORING */
    shm_id_config = string_to_int(argv[0]);
    shm_id_ports_coords = string_to_int(argv[1]);
    mq_id_request = string_to_int(argv[2]);
    sem_id_generation = string_to_int(argv[3]);


    if((shm_cfg = shmat(shm_id_config, NULL, SHM_RDONLY)) == (void*) -1) {
        perror("[NAVE] Error while trying to attach to configuration shared memory");
        kill(getppid(), SIGINT);
    }

    if((shm_ports_coords = shmat(shm_id_ports_coords, NULL, SHM_RDONLY)) == (void*) -1) {
        perror("[NAVE] Error while trying to attach to ports coordinates shared memory");
        kill(getppid(), SIGINT);
    }

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
    id_destination_port = -1;

    sa.sa_handler = nave_sig_handler;
    sa.sa_flags = SA_RESTART;

    sigaction(SIGALRM, &sa, NULL);

    /* TODO: The ship can't exit on its own, it has to be killed by the master or weather */
    if(sem_cmd(sem_id_generation, 0, -1, 0) < 0) {
        perror("[NAVE] Error while trying to release sem_id_generation");
        kill(getppid(), SIGINT);
    }

    while (1) {
        old_id_destination_port = id_destination_port;
        do {
            id_destination_port = (int) random() % shm_cfg->SO_PORTI;
        } while (id_destination_port == old_id_destination_port);

        move(shm_ports_coords[id_destination_port]);
    }

}

void nave_sig_handler(int signum) {
    switch (signum) {
        case SIGALRM:
            break;
    }
}
