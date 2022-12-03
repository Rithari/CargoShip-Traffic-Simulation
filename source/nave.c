#include "../headers/master.h"
#include "../headers/utils.h"
#include "../headers/common_ipcs.h"
#include <math.h>

/*se una nave non ha banchine e code di attracco libere allora viene gettata in mare*/
/*nave deve ricordarsi l'ultimo porto di partenza per evitare che ci ritorni quando viene messa in mare */
/*una nave in mare cerca sempre una coda di attracco libera (BUSY WAITING!!)*/

void nave_sig_handler(int);

int actual_capacity;
coord actual_coordinate;

void move(config *cfg, coord destination) {
    struct timespec ts, rem;
    struct timespec start, end;

    double dx = destination.x - actual_coordinate.x;
    double dy = destination.y - actual_coordinate.y;

    /*distance / SO_SPEED*/
    /* double navigation_time = sqrt(dx * dx + dy * dy) / cfg->SO_SPEED; +/

    printf("Navigation time: %f\n", navigation_time); */
    ts.tv_sec = 0;
    ts.tv_nsec = 0;

    while (nanosleep(&ts, &rem) && errno != EINVAL) {
        switch (errno) {
            case EFAULT:
                perror("nave.c: Problem with copying information from user space.");
                exit(EXIT_FAILURE);
            case EINTR:
                clock_gettime(CLOCK_REALTIME, &start);
                perror("nave.c");
                /* TODO: aggiungere funzionalità */
                clock_gettime(CLOCK_REALTIME, &end);
                timespec_sub(&end, &end, &start);
                timespec_sub(&rem, &rem, &end);
                ts = rem;
                continue;
            default:
                perror("Generic error in nave.c");
                exit(EXIT_FAILURE);
        }
    }

    actual_coordinate = destination;
}

int main(void) {
    config *shm_cfg;
    coord c;
    struct sigaction sa;
    int shm_id;

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

    sleep(1);

    actual_capacity = 0;
    actual_coordinate.x = 0;
    actual_coordinate.y = 0;
    sa.sa_handler = nave_sig_handler;

    sigaction(SIGTERM, &sa, NULL);

    c.x = 1000;
    c.y = 1000;

    move(shm_cfg, c);

    /* TODO: The ship can't exit on its own, it has to be killed by the master or weather */
    /* So make it wait for input */

}

void nave_sig_handler(int sigsum) {
    sigsum++;
}
