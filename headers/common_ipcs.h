#ifndef PROGETTOSO_COMMON_IPCS_H
#define PROGETTOSO_COMMON_IPCS_H

#include "master.h"

typedef struct {
    long    mtype;
    int     quantity;
    int     requestingPort;
} requestMessage;

typedef struct {
    long    mtype;
    int     quantity;
    int     lifespan;
} offerMessage;

/* Functions to handle semaphores, the latter is timed */
int sem_cmd(int s_id, unsigned short sem_num, short sem_op, short sem_flg);
int sem_timed_cmd(int s_id, unsigned short sem_num, short sem_op, short sem_flg, struct timespec *tv);

#endif /*PROGETTOSO_COMMON_IPCS_H*/
