#ifndef PROGETTOSO_COMMON_IPCS_H
#define PROGETTOSO_COMMON_IPCS_H

#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include "../headers/master.h"

#define KEY_CONFIG      ftok(PATH_CONFIG, 0x01)
#define KEY_MQ_REQUESTS ftok(PATH_CONFIG, 0x02)
#define KEY_MQ_PATHS    ftok(PATH_CONFIG, 0x03)
#define KEY_SEM         ftok(PATH_CONFIG, 0x04)
#define KEY_GOODS_TON   ftok(PATH_CONFIG, 0x05)



int initialize_message_queue(int key);
int initialize_semaphore(int key, int n_semaphores);
void detach_all(void);

#endif /*PROGETTOSO_COMMON_IPCS_H*/
