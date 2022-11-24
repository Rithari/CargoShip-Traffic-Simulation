#ifndef PROGETTOSO_COMMON_IPCS_H
#define PROGETTOSO_COMMON_IPCS_H

#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include "../headers/master.h"

#define KEY_CONFIG      ftok(PATH_CONFIG, 0x01)
#define KEY_GOODS_TON   ftok(PATH_CONFIG, 0x02)



#endif /*PROGETTOSO_COMMON_IPCS_H*/
