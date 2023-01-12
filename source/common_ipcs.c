#include "../headers/common_ipcs.h"

/* Semaphore operations */

int sem_cmd(int s_id, unsigned short sem_num, short sem_op, short sem_flg) {
    struct sembuf sops;
    sops.sem_flg = sem_flg;
    sops.sem_op = sem_op;
    sops.sem_num = sem_num;
    return semop(s_id, &sops, 1);
}
/*
int sem_timed_cmd(int s_id, unsigned short sem_num, short sem_op, short sem_flg, struct timespec *tv) {
    struct sembuf sops;
    sops.sem_flg = sem_flg;
    sops.sem_op = sem_op;
    sops.sem_num = sem_num;
    return semtimedop(s_id, &sops, 1, tv);
} */
