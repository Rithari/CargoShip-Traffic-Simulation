#include "../headers/master.h"
#include "../headers/utils.h"
#include "../headers/common_ipcs.h"

#define BUFFER_SIZE 128

void master_sig_handler(int signum);

pid_t *ships_pid;
pid_t *ports_pid;
config *shm_cfg;


int main(int argc, char** argv) {
    struct sigaction sa;
    int shm_id;
    int requests_id;
    int paths_id;

    if((shm_id = shmget(KEY_CONFIG, sizeof(*shm_cfg), IPC_CREAT | IPC_EXCL | 0600)) < 0) {
        if(errno == EEXIST) {
            shm_id = shmget(KEY_CONFIG, sizeof(*shm_cfg), 0600);
            shmctl(shm_id, IPC_RMID, NULL);
            shm_id = shmget(KEY_CONFIG, sizeof(*shm_cfg), IPC_CREAT | IPC_EXCL | 0600);
        } else {
            printf("Error during master->shmget() for config while checking for duplicates.\n");
            exit(EXIT_FAILURE);
        }
    }

    /* Attach the shared memory segment and update cfg pointer */
    if((shm_cfg = shmat(shm_id, NULL, 0)) == (void *) -1) {
        printf("Error during master->shmat() for config.\n");
        exit(EXIT_FAILURE);
    }

    requests_id = initialize_message_queue(KEY_MQ_REQUESTS);
    paths_id = initialize_message_queue(KEY_MQ_PATHS);
    initialize_so_vars(shm_cfg, PATH_CONFIG);

    /* Array of pids for the children */
    ships_pid = malloc(sizeof(pid_t) * shm_cfg->SO_NAVI);
    ports_pid = malloc(sizeof(pid_t) * shm_cfg->SO_PORTI);
    print_config(shm_cfg);


    sa.sa_handler = master_sig_handler;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGCHLD, &sa, NULL);

    /* TODO: wait for ports to finish generating before generating ships.*/
    create_ports(shm_cfg, ports_pid);
    create_ships(shm_cfg, ships_pid);
    /* TODO: When a child is killed, it should be "reborn" and its pid replaced in the array */
    /* Can be handled with a signal handler */

    while (wait(NULL) > 0)
    {
        /* code */
        /* che code? quelle per il dubai alle 13? */
    }


    /* You only get here when SO_DAYS have passed and all processes are killed */
    shmctl(shm_id, IPC_RMID, NULL);
    msgctl(requests_id, IPC_RMID, NULL);
    msgctl(paths_id, IPC_RMID, NULL);

    free(ships_pid);
    free(ports_pid);
}

void initialize_so_vars(config *cfg, char* path_cfg_file) {
    /* Configuration file setup */
    FILE *fp;
    char buffer[BUFFER_SIZE];

    fp = fopen(path_cfg_file, "r");
    if(!fp) {
        perror("Error during: initialize_so_vars->fopen()");
        exit(EXIT_FAILURE);
    }

    cfg->check = 0;

    while (!feof(fp)) {
        if(fgets(buffer, BUFFER_SIZE, fp) == NULL) {
            break;
        }

        if(buffer[0] == '#') {
            continue;
        }


        if(sscanf(buffer, "SO_NAVI: %d", &cfg->SO_NAVI) == 1) {
            cfg->check |= 1;
        } else if(sscanf(buffer, "SO_PORTI: %d", &cfg->SO_PORTI) == 1) {
            cfg->check |= 1 << 1;
        } else if(sscanf(buffer, "SO_MERCI: %d", &cfg->SO_MERCI) == 1) {
            cfg->check |= 1 << 2;
        } else if(sscanf(buffer, "SO_SIZE: %d", &cfg->SO_SIZE) == 1) {
            cfg->check |= 1 << 3;
        } else if(sscanf(buffer, "SO_MIN_VITA: %d", &cfg->SO_MIN_VITA) == 1) {
            cfg->check |= 1 << 4;
        } else if(sscanf(buffer, "SO_MAX_VITA: %d", &cfg->SO_MAX_VITA) == 1) {
            cfg->check |= 1 << 5;
        } else if(sscanf(buffer, "SO_LATO: %lf", &cfg->SO_LATO) == 1) {
            cfg->check |= 1 << 6;
        } else if(sscanf(buffer, "SO_SPEED: %d", &cfg->SO_SPEED) == 1) {
            cfg->check |= 1 << 7;
        } else if(sscanf(buffer, "SO_CAPACITY: %d", &cfg->SO_CAPACITY) == 1) {
            cfg->check |= 1 << 8;
        } else if(sscanf(buffer, "SO_BANCHINE: %d", &cfg->SO_BANCHINE) == 1) {
            cfg->check |= 1 << 9;
        } else if(sscanf(buffer, "SO_FILL: %d", &cfg->SO_FILL) == 1) {
            cfg->check |= 1 << 10;
        } else if(sscanf(buffer, "SO_LOADSPEED: %d", &cfg->SO_LOADSPEED) == 1) {
            cfg->check |= 1 << 11;
        } else if(sscanf(buffer, "STEP: %d", &cfg->STEP) == 1) {
            cfg->check |= 1 << 12;
        } else if(sscanf(buffer, "SO_DAYS: %d", &cfg->SO_DAYS) == 1) {
            cfg->check |= 1 << 13;
        } else if(sscanf(buffer, "TOPK: %d", &cfg->TOPK) == 1) {
            cfg->check |= 1 << 14;
        } else if(sscanf(buffer, "STORM_DURATION: %d", &cfg->STORM_DURATION) == 1) {
            cfg->check |= 1 << 15;
        } else if(sscanf(buffer, "SWELL_DURATION: %d", &cfg->SWELL_DURATION) == 1) {
            cfg->check |= 1 << 16;
        } else if(sscanf(buffer, "ML_INTENSITY: %d", &cfg->ML_INTENSITY) == 1) {
            cfg->check |= 1 << 17;
        }
    }
    fclose(fp);

    if(cfg->check != 0x3FFFF) {
        errno = EINVAL;
        perror("Missing config");
        exit(EXIT_FAILURE);
    }

    if(cfg->SO_NAVI < 1) {
        errno = EINVAL;
        perror("SO_NAVI is less than 1");
        exit(EXIT_FAILURE);
    }

    if(cfg->SO_PORTI < 4) {
        errno = EINVAL;
        perror("SO_PORTI is less than 4");
        exit(EXIT_FAILURE);
    }
}

void create_ships(config *cfg, pid_t *ships) {
    int i;

    for(i = 0; i < cfg->SO_NAVI; i++) {
        switch(ships[i] = fork()) {
            case -1:
                perror("Error during: create_ships->fork()");
                exit(EXIT_FAILURE);
            case 0:
                execv(PATH_NAVE, NULL);
                perror("execv has failed trying to run the ship");
                exit(EXIT_FAILURE);
            default:
                break;
        }
    }
}

void create_ports(config *cfg, pid_t *ports) {
    int i;

    /* TODO: Create the first 4 ports in the map's 4 corners */
    for(i = 0; i < cfg->SO_PORTI; i++) {
        switch(ports[i] = fork()) {
            case -1:
                perror("Error during: create_ports->fork()");
                exit(EXIT_FAILURE);
            case 0:
                execv(PATH_PORTO, NULL);
                perror("execv has failed trying to run port");
                exit(EXIT_FAILURE);
            default:
                break;
        }
    }
}

int initialize_message_queue(int key) {
    int mq_id;
    if((mq_id = msgget(key, IPC_CREAT | IPC_EXCL | 0666)) < 0) {
        if (errno == EEXIST) {
            mq_id = msgget(key, 0666);
            msgctl(mq_id, IPC_RMID, NULL);
            mq_id = msgget(key, IPC_CREAT | IPC_EXCL | 0666);
        } else {
            printf("error during initialization of message queue\n");
            exit(EXIT_FAILURE);
        }
    }
    return mq_id;
}

/* TODO: SIGALRM dopo SO_DAYS bisogna killare tutto, prima salvando tutti i dati per statistica */


void master_sig_handler(int signum) {
    int i;
    int status;
    pid_t killed_id;
    switch(signum) {
        case SIGINT:
            printf("SIGINT received, killing all processes.\n");
            for(i = 0; i < shm_cfg->SO_NAVI; i++) {
                kill(ships_pid[i], SIGTERM);
            }
            for(i = 0; i < shm_cfg->SO_PORTI; i++) {
                kill(ports_pid[i], SIGTERM);
            }
            break;
        case SIGALRM:

            break;
        case SIGCHLD:
            /* Waitpid to capture recently exited children's exit code */
            while((killed_id = waitpid(-1, &status, 0)) > 0) {
                printf("Child has exited with status %d\n", WEXITSTATUS(status));
                if(WEXITSTATUS(status) == EXIT_DEATH) {
                    pid_t replacement_id;
                    printf("A ship has died. Restarting it and replacing its PID in the array.\n");

                    switch(replacement_id = fork()) {
                        case -1:
                            perror("Error during: master_sig_handler->fork()");
                            exit(EXIT_FAILURE);
                        case 0:
                            /* Find killed_id in the array and replace it with the new one */
                            for(i = 0; i < shm_cfg->SO_NAVI; i++) {
                                if(ships_pid[i] == killed_id) {
                                    ships_pid[i] = replacement_id;
                                    break;
                                }
                            }
                            execv(PATH_NAVE, NULL);
                            perror("execv has failed trying to run the ship");
                            exit(EXIT_FAILURE);
                        default:
                            break;
                    }
                }
            }
            break;
        default:
            break;
    }
}
