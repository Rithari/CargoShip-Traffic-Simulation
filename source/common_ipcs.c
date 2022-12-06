#include "../headers/common_ipcs.h"

int initialize_message_queue(int key) {
    int mq_id;
    if((mq_id = msgget(key, IPC_CREAT | IPC_EXCL | 0600)) < 0) {
        if (errno == EEXIST) {
            mq_id = msgget(key, 0600);
            msgctl(mq_id, IPC_RMID, NULL);
            mq_id = msgget(key, IPC_CREAT | IPC_EXCL | 0600);
        } else {
            printf("error during initialization of message queue\n");
            exit(EXIT_FAILURE);
        }
    }
    return mq_id;
}

int initialize_semaphore(int key, int n_semaphores) {
    int id;
    if ((id = semget(key, n_semaphores, IPC_CREAT | IPC_EXCL | 0600)) < 0 ) {
        if(errno == EEXIST) {
            printf("Semaphore already exists. Trying to get it...\n");
            id = semget(key, n_semaphores, 0600);
            semctl(id, 0, IPC_RMID);
            id = semget(key, n_semaphores, IPC_CREAT | IPC_EXCL | 0600);
            printf("Semaphore created with id: %d", id);
        } else {
            printf("Error during semget() initialization. Exiting...\n");
            exit(EXIT_FAILURE);
        }
    }
    return id;
}

void detach_all(int shm_id, int requests_id, int paths_id, int *ships_pid, int *ports_pid) {
    shmctl(shm_id, IPC_RMID, NULL);
    msgctl(requests_id, IPC_RMID, NULL);
    msgctl(paths_id, IPC_RMID, NULL);

    free(ships_pid);
    free(ports_pid);
}

void sem_lock(int sem_id, int sem_num) {
    struct sembuf sop;
    sop.sem_num = sem_num;
    sop.sem_op = -1;
    sop.sem_flg = 0;
    if(semop(sem_id, &sop, 1) < 0) {
        perror("Error during sem_lock()");
        exit(EXIT_FAILURE);
    }
}

void sem_unlock(int sem_id, int sem_num) {
    struct sembuf sop;
    sop.sem_num = sem_num;
    sop.sem_op = 1;
    sop.sem_flg = 0;
    if(semop(sem_id, &sop, 1) < 0) {
        perror("Error during sem_unlock()");
        exit(EXIT_FAILURE);
    }
}

void sem_lock_nowait(int sem_id, int sem_num) {
    struct sembuf sop;
    sop.sem_num = sem_num;
    sop.sem_op = -1;
    sop.sem_flg = IPC_NOWAIT;
    if(semop(sem_id, &sop, 1) < 0) {
        perror("Error during sem_lock_nowait()");
        exit(EXIT_FAILURE);
    }
}

void sem_unlock_nowait(int sem_id, int sem_num) {
    struct sembuf sop;
    sop.sem_num = sem_num;
    sop.sem_op = 1;
    sop.sem_flg = IPC_NOWAIT;
    if(semop(sem_id, &sop, 1) < 0) {
        perror("Error during sem_unlock_nowait()");
        exit(EXIT_FAILURE);
    }
}
