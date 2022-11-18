#include "master.h"

/*se una nave non ha banchine e code di attracco libere allora viene gettata in mare*/
/*nave deve ricordarsi l'ultimo porto di partenza per evitare che ci ritorni quando viene messa in mare */
/*una nave in mare cerca sempre una coda di attracco libera (BUSY WAITING!!)*/

void nave_sig_handler(int);

int actual_capacity;
coord coordinate;

void move(coord destination) {
    struct timespec ts, rem;
    struct timespec start, end;

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
                /*TODO: aggiungere funzionalità*/
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

    coordinate = destination;
}

int main(void) {
    struct sigaction sa;
    actual_capacity = 0;
    coordinate.x = 0;
    coordinate.y = 0;
    sa.sa_handler = nave_sig_handler;

    sigaction(SIGTERM, &sa, NULL);

    return 0;
}

void nave_sig_handler(int sigsum) {

}