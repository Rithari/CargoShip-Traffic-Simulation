#include "../headers/master.h"
#include "../headers/utils.h"
#include "../headers/common_ipcs.h"

#define BUFFER_SIZE 128

void master_sig_handler(int signum);

pid_t *ships_pid;
pid_t *ports_pid;
config *shm_cfg;
int shm_id;
int requests_id;
int paths_id;


int main(void) {
    struct sigaction sa;
    int i;

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
    sa.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGCHLD, &sa, NULL);
    sigaction(SIGALRM, &sa, NULL);

    /* TODO: wait for ports to finish generating before generating ships.*/
    create_ports(shm_cfg);
    create_ships(shm_cfg);

    alarm(1);
    while(waitpid(-1, NULL, 0) > 0) {
        switch (errno) {
            case EINTR:
                printf("I'm in eintr.");
                continue;
            default:
                perror("switch wait");
        }
    }

    /* You only get here when SO_DAYS have passed and all processes are killed */
    detach_all();
    return 0;
}


void detach_all(void) {
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

    cfg->CURRENT_DAY = 0;

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

void create_ships(config *cfg) {
    int i;

    for(i = 0; i < cfg->SO_NAVI; i++) {
        switch(ships_pid[i] = fork()) {
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

void create_ports(config *cfg) {
    int i, j;
    char s_x[BUFFER_SIZE], s_y[BUFFER_SIZE];
    coord *ports_coords = malloc(sizeof(*ports_coords) * cfg->SO_PORTI);

    ports_coords[0].x = 0;
    ports_coords[0].y = 0;
    ports_coords[1].x = cfg->SO_LATO;
    ports_coords[1].y = 0;
    ports_coords[2].x = cfg->SO_LATO;
    ports_coords[2].y = cfg->SO_LATO;
    ports_coords[3].x = 0;
    ports_coords[3].y = cfg->SO_LATO;

    for(i = 4; i < cfg->SO_PORTI; i++) {
        double rndx = (double) random() / RAND_MAX * cfg->SO_LATO;
        double rndy = (double) random() / RAND_MAX * cfg->SO_LATO;

        for(j = 0; j < i; j++) {
            if(ports_coords[j].x == rndx && ports_coords[j].y == rndy) {
                i--;
            } else {
                ports_coords[i].x = rndx;
                ports_coords[i].y = rndy;
            }
        }
    }

    /* TODO: Create the first 4 ports in the map's 4 corners */
    /* Pass arguments to the port process to tell it where to create the port */
    for(i = 0; i < cfg->SO_PORTI; i++) {
        switch(ports_pid[i] = fork()) {
            case -1:
                perror("Error during: create_ports->fork()");
                exit(EXIT_FAILURE);
            case 0:
                snprintf(s_x, BUFFER_SIZE, "%lf", ports_coords[i].x);
                snprintf(s_y, BUFFER_SIZE, "%lf", ports_coords[i].y);
                execl(PATH_PORTO, s_x, s_y, NULL);
                perror("execv has failed trying to run port");
                exit(EXIT_FAILURE);
            default:
                break;
        }
    }
}

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

void master_sig_handler(int signum) {
    int old_errno = errno;
    int i;
    int status;
    pid_t killed_id;

    switch(signum) {
        case SIGTERM:
        case SIGINT:
            printf("SIGINT received, killing all processes.\n");
            for(i = 0; i < shm_cfg->SO_NAVI; i++) {
                kill(ships_pid[i], SIGTERM);
            }
            for(i = 0; i < shm_cfg->SO_PORTI; i++) {
                kill(ports_pid[i], SIGTERM);
            }
            detach_all();
            exit(EXIT_FAILURE);
        /* Still needs to deal with statistics first */
        case SIGALRM:
            /* Remota possibilità di concorrenza in shm_cfg->CURRENT_DAY */
            shm_cfg->CURRENT_DAY++;
            printf("Day [%d]/[%d].\n", shm_cfg->CURRENT_DAY, shm_cfg->SO_DAYS);

            /* Check SO_DAYS against the current day. If they're the same kill everything */
            if(shm_cfg->SO_DAYS == shm_cfg->CURRENT_DAY) {
                printf("Reached SO_DAYS, killing all processes.\n");
                for(i = 0; i < shm_cfg->SO_NAVI; i++) {
                    kill(ships_pid[i], SIGTERM);
                }
                for(i = 0; i < shm_cfg->SO_PORTI; i++) {
                    kill(ports_pid[i], SIGTERM);
                }
            } else {
                for(i = 0; i < shm_cfg->SO_NAVI; i++) {
                    kill(ships_pid[i], SIGALRM);
                }
                for(i = 0; i < shm_cfg->SO_PORTI; i++) {
                    kill(ports_pid[i], SIGALRM);
                }
            }
            alarm(1);
            break;
        case SIGCHLD:
            /* Waitpid to capture recently exited children's exit code */
            while((killed_id = waitpid(-1, &status, 0)) > 0) {
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
    errno = old_errno;
}
