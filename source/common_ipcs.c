#include "../headers/common_ipcs.h"

int sem_cmd(int s_id, unsigned short sem_num, short sem_op, short sem_flg) {
    struct sembuf sops;
    sops.sem_flg = sem_flg;
    sops.sem_op = sem_op;
    sops.sem_num = sem_num;
    return semop(s_id, &sops, 1);
}

int set_mutex_sem_array(int sem_id, int sem_len) {
    int ret;
    unsigned short *sem_value = malloc(sizeof(*sem_value) * sem_len);

    memset(sem_value, 1, sizeof(unsigned short) * sem_len);
    ret = semctl(sem_id, 0, SETALL, sem_value);
    free(sem_value);
    return ret;
}
