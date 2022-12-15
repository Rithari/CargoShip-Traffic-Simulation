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
    /* struct timespec start, end; */

    double dx = destination.x - actual_coordinate.x;
    double dy = destination.y - actual_coordinate.y;

    /*distance / SO_SPEED*/
    double navigation_time = sqrt(dx * dx + dy * dy) / shm_cfg->SO_SPEED;

    ts.tv_sec = (long) navigation_time;
    ts.tv_nsec = (long) ((navigation_time - ts.tv_sec) * 1000000000);

    while (nanosleep(&ts, &rem)) {
        switch (errno) {
            case EINTR:

                perror("nave.c");
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
    int old_errno = errno;
    switch (signum) {
        case SIGALRM:
            break;
    }
    errno = old_errno;
}

/*
 * This function uses a while loop to keep trying to pick a
 * random port and acquire a lock on its dock until it succeeds.
 * It first selects a random port from the coordinates array and gets
 * the semaphore ID associated with that port. It then tries to acquire a
 * lock on the port's semaphore by checking its value and decrementing it
 * if it is greater than 0. If the port's semaphore is not available (its value is 0),
 * the function simply selects a different port and tries again. If the port's semaphore is available,
 * the function decrements the semaphore value to acquire a lock on the dock and returns the index of the port.
*/
int pick_rand_port() {
    // Keep trying to pick a random port and acquire a lock on its dock until we succeed
    int port_index = -1;
    int port_semaphore_id;
    int i = 0;
    struct sembuf semaphore_operation;

    while (port_index == -1) {
        // Select a random port from the coordinates array
        port_index = i;
        i = (i + 1) % shm_cfg->SO_PORTI;
        port_semaphore_id = shm_ports_coords[port_index].semaphore_id;

        // Try to acquire a lock on the port's semaphore
        // Decrement the port's semaphore value to acquire a lock on the dock
        semaphore_operation.sem_num = 0;
        semaphore_operation.sem_op = -1;
        semaphore_operation.sem_flg = IPC_NOWAIT;
        if (semop(port_semaphore_id, &semaphore_operation, 1) == -1 ) {
            // The port's semaphore is not available (its value is 0)
            switch(errno) {
                case EAGAIN:
                    port_index = -1;
                    // The port's semaphore is not available (its value is 0)
                    // Select a different port and try again
                    continue;
                default:
                    // Generic error
                    perror("Error in pick_rand_port()");
                    exit(EXIT_FAILURE);
            }

            // Return the index of the port we selected
            return port_index;
        }
    }
}
