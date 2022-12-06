#include "../headers/master.h"
#include "../headers/utils.h"
#include "../headers/common_ipcs.h"
#include <math.h>

/*se una nave non ha banchine e code di attracco libere allora viene gettata in mare*/
/*nave deve ricordarsi l'ultimo porto di partenza per evitare che ci ritorni quando viene messa in mare */
/*una nave in mare cerca sempre una coda di attracco libera (BUSY WAITING!!)*/

void nave_sig_handler(int);
void print_time(struct timespec *ts);
void move(config *cfg, coord destination);

int actual_capacity;
coord actual_coordinates;
config *shm_cfg;

int main(void) {
    coord c;
    struct sigaction sa;
    int shm_id;
    srandom(getpid());

    /* printf("KEY_CONFIG: %d\n", KEY_CONFIG); */

    /* TODO: Attach to message queues */

    if((shm_id = shmget(KEY_CONFIG, sizeof(*shm_cfg), 0600)) < 0) {
        perror("Error during nave->shmget()");
        exit(EXIT_FAILURE);
    }

    if((shm_cfg = shmat(shm_id, NULL, SHM_RDONLY)) == (void*) -1) {
        perror("Error during nave->shmat()");
        exit(EXIT_FAILURE);
    }

    actual_coordinates.x = (double) random() / RAND_MAX * shm_cfg->SO_LATO;
    actual_coordinates.y = (double) random() / RAND_MAX * shm_cfg->SO_LATO;

    /*printf("[%d] Generated ship.\n", getpid());*/

    actual_capacity = 0;
    actual_coordinates.x = 0;
    actual_coordinates.y = 0;
    sa.sa_handler = nave_sig_handler;
    sa.sa_flags = SA_RESTART;

    sigaction(SIGALRM, &sa, NULL);

    /* TODO: The ship can't exit on its own, it has to be killed by the master or weather */
    /* So make it wait for input */

    while (1) {
        c.x = (double) random() / RAND_MAX *  shm_cfg->SO_LATO;
        c.y = (double) random() / RAND_MAX *  shm_cfg->SO_LATO;
        move(shm_cfg, c);
    }

}

void move(config *cfg, coord destination) {
    struct timespec ts, rem;
    /* struct timespec start, end; */

    double dx = destination.x - actual_coordinates.x;
    double dy = destination.y - actual_coordinates.y;

    /*distance / SO_SPEED*/
    double navigation_time = sqrt(dx * dx + dy * dy) / cfg->SO_SPEED;

    printf("Navigation time: %f\n", navigation_time);



    ts.tv_sec = (long) navigation_time;
    ts.tv_nsec = (navigation_time - ts.tv_sec) * 1000000000;

    while (nanosleep(&ts, &rem) && errno != EINVAL) {
        switch (errno) {
            case EFAULT:
                perror("nave.c: Problem with copying information from user space.");
                exit(EXIT_FAILURE);
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
                print_time(&ts);
                continue;
            default:
                perror("Generic error in nave.c");
                exit(EXIT_FAILURE);
        }
    }
    printf("Moved from [%lf, %lf] to [%lf, %lf].\n\n", actual_coordinates.x, actual_coordinates.y, destination.x, destination.y);
    fflush(stdout);
    actual_coordinates = destination;
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
        port_semaphore_id = ports_coords[port_index].semaphore_id;

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



void print_time(struct timespec *ts) {
    printf("SECONDS:            %ld\n", ts->tv_sec);
    printf("NANOSECONDS:        %ld\n", ts->tv_nsec);
}

void nave_sig_handler(int signum) {
    switch (signum) {
        case SIGALRM:
            printf("NAVE\n");
            break;
        default:
            break;
    }
}
