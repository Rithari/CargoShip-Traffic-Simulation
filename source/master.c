#include "../headers/master.h"
#include "../headers/utils.h"
#include "../headers/common_ipcs.h"
#include "../headers/linked_list.h"

#define BUFFER_SIZE 128

/* initialize @*cfg with parameters wrote on @*path_cfg_file*/
void initialize_so_vars(char* path_cfg_file);
void initialize_ports_coords(void);

/* this methods shouldn't be here */
void initialize_semaphores(int sem_id);

void create_ships(void);
void create_ports(void);
void create_weather(void);

/* clear all the memory usage */
void clear_all(void);

/* signal handler */
void master_sig_handler(int signum);

pid_t   *ships_pid;
pid_t   *ports_pid;
config  *shm_cfg;
coord   *shm_ports_coords;
int     shm_id_config;
int     shm_id_ports_coords;
int     mq_id_request;
int     mq_id_ships;
int     mq_id_ports;
int     sem_id_generation;


int main(int argc, char **argv) {
    struct timespec tv;
    struct sigaction sa;
    int i;

    if(argc != 2) {
        printf("[MASTER] Incorrect number of parameters. Re-execute with: {configuration file}\n");
        exit(EXIT_FAILURE);
    }

    /* create and attach config shared memory segment */
    if ((shm_id_config = shmget(IPC_PRIVATE, sizeof(*shm_cfg), 0600)) < 0) {
        perror("[MASTER] Error while creating shared memory for configuration parameters");
        exit(EXIT_FAILURE);
    }

    if ((shm_cfg = shmat(shm_id_config, NULL, 0)) == (void *) -1) {
        perror("[MASTER] Error while trying to attach to configuration shared memory");
        raise(SIGINT);
    }
    /* bruh */
    initialize_so_vars(argv[1]);

    /* create and attach ports coordinate shared memory segment */
    if ((shm_id_ports_coords = shmget(IPC_PRIVATE, sizeof(*shm_ports_coords) * shm_cfg->SO_PORTI, 0600)) < 0) {
        perror("[MASTER] Error while creating shared memory for ports coordinates");
        raise(SIGINT);
    }

    if ((shm_ports_coords = shmat(shm_id_ports_coords, NULL, 0)) == (void *) -1) {
        perror("[MASTER] Error while trying to attach to ports coordinates shared memory");
        raise(SIGINT);
    }

    initialize_ports_coords();

    if ((mq_id_request = msgget(IPC_PRIVATE, 0600)) < 0) {
        perror("[MASTER] Error while creating message queue for requests");
        raise(SIGINT);
    }

    if ((sem_id_generation = semget(IPC_PRIVATE, 1, 0600)) < 0) {
        perror("[MASTER] Error while creating semaphore for control generation order");
        raise(SIGINT);
    }

    /* Array of pids for the children */
    ships_pid = malloc(sizeof(pid_t) * shm_cfg->SO_NAVI);
    ports_pid = malloc(sizeof(pid_t) * shm_cfg->SO_PORTI);
    /*print_config(shm_cfg);*/

    sa.sa_handler = master_sig_handler;
    sa.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGCHLD, &sa, NULL);
    sigaction(SIGALRM, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    /* sem_timed_cmd need those settings, value is arbitrary. */
    tv.tv_sec = 2;
    tv.tv_nsec = 0;

    /* TODO: with this method u can "only" generate MAX_SHORT ports... Maybe it's a problem maybe not */
    sem_cmd(sem_id_generation, 0, shm_cfg->SO_PORTI, 0);
    create_ports();
    printf("Waiting for ports generation...\n");
    if(sem_cmd(sem_id_generation, 0, 0, 0) < 0) {
        perror("[MASTER] Error while waiting for ports generation");
        raise(SIGINT);
    }

    sem_cmd(sem_id_generation, 0, shm_cfg->SO_NAVI, 0);
    create_ships();
    printf("Waiting for ships generation...\n");
    if(sem_cmd(sem_id_generation, 0, 0, 0) < 0) {
        perror("[MASTER] Error while waiting for ships generation");
        raise(SIGINT);
    }

    alarm(1);
    while(waitpid(-1, NULL, 0) > 0) {
        switch (errno) {
            case EINTR:
                printf("I'm in eintr.");
                continue;
            default:
                perror("switch wait");
                break;
        }
    }

    /* You only get here when SO_DAYS have passed or all ships processes are killed */
    clear_all();
    return 0;
}

void clear_all(void) {
    shmctl(shm_id_config, IPC_RMID, NULL);
    shmctl(shm_id_ports_coords, IPC_RMID, NULL);
    msgctl(mq_id_request, IPC_RMID, NULL);
    semctl(sem_id_generation, 0, IPC_RMID, NULL);
    free(ships_pid);
    free(ports_pid);
}

void initialize_so_vars(char* path_cfg_file) {
    /* Configuration file setup */
    FILE *fp;
    char buffer[BUFFER_SIZE];

    fp = fopen(path_cfg_file, "r");
    if(!fp) {
        perror("Error during: initialize_so_vars->fopen()");
        exit(EXIT_FAILURE);
    }

    shm_cfg->check = 0;

    while (!feof(fp)) {
        if(fgets(buffer, BUFFER_SIZE, fp) == NULL) {
            break;
        }

        if(sscanf(buffer, "#%s", buffer) == 1) {
            continue;
        }

        if(sscanf(buffer, "SO_NAVI: %d", &shm_cfg->SO_NAVI) == 1) {
            shm_cfg->check |= 1;
        } else if(sscanf(buffer, "SO_PORTI: %d", &shm_cfg->SO_PORTI) == 1) {
            shm_cfg->check |= 1 << 1;
        } else if(sscanf(buffer, "SO_MERCI: %d", &shm_cfg->SO_MERCI) == 1) {
            shm_cfg->check |= 1 << 2;
        } else if(sscanf(buffer, "SO_SIZE: %d", &shm_cfg->SO_SIZE) == 1) {
            shm_cfg->check |= 1 << 3;
        } else if(sscanf(buffer, "SO_MIN_VITA: %d", &shm_cfg->SO_MIN_VITA) == 1) {
            shm_cfg->check |= 1 << 4;
        } else if(sscanf(buffer, "SO_MAX_VITA: %d", &shm_cfg->SO_MAX_VITA) == 1) {
            shm_cfg->check |= 1 << 5;
        } else if(sscanf(buffer, "SO_LATO: %lf", &shm_cfg->SO_LATO) == 1) {
            shm_cfg->check |= 1 << 6;
        } else if(sscanf(buffer, "SO_SPEED: %d", &shm_cfg->SO_SPEED) == 1) {
            shm_cfg->check |= 1 << 7;
        } else if(sscanf(buffer, "SO_CAPACITY: %d", &shm_cfg->SO_CAPACITY) == 1) {
            shm_cfg->check |= 1 << 8;
        } else if(sscanf(buffer, "SO_BANCHINE: %d", &shm_cfg->SO_BANCHINE) == 1) {
            shm_cfg->check |= 1 << 9;
        } else if(sscanf(buffer, "SO_FILL: %d", &shm_cfg->SO_FILL) == 1) {
            shm_cfg->check |= 1 << 10;
        } else if(sscanf(buffer, "SO_LOADSPEED: %d", &shm_cfg->SO_LOADSPEED) == 1) {
            shm_cfg->check |= 1 << 11;
        } else if(sscanf(buffer, "SO_DAYS: %d", &shm_cfg->SO_DAYS) == 1) {
            shm_cfg->check |= 1 << 12;
        } else if(sscanf(buffer, "STORM_DURATION: %d", &shm_cfg->STORM_DURATION) == 1) {
            shm_cfg->check |= 1 << 13;
        } else if(sscanf(buffer, "SWELL_DURATION: %d", &shm_cfg->SWELL_DURATION) == 1) {
            shm_cfg->check |= 1 << 14;
        } else if(sscanf(buffer, "ML_INTENSITY: %d", &shm_cfg->ML_INTENSITY) == 1) {
            shm_cfg->check |= 1 << 15;
        }
    }
    fclose(fp);

    shm_cfg->CURRENT_DAY = 0;

    if(shm_cfg->check != 0xFFFF) {
        errno = EINVAL;
        perror("Missing config");
        exit(EXIT_FAILURE);
    }

    if(shm_cfg->SO_NAVI < 1) {
        errno = EINVAL;
        perror("SO_NAVI is less than 1");
        exit(EXIT_FAILURE);
    }

    if(shm_cfg->SO_PORTI < 4) {
        errno = EINVAL;
        perror("SO_PORTI is less than 4");
        exit(EXIT_FAILURE);
    }
}

void initialize_ports_coords(void) {
    int i, j;

    shm_ports_coords[0].x = 0;
    shm_ports_coords[0].y = 0;
    shm_ports_coords[1].x = shm_cfg->SO_LATO;
    shm_ports_coords[1].y = 0;
    shm_ports_coords[2].x = shm_cfg->SO_LATO;
    shm_ports_coords[2].y = shm_cfg->SO_LATO;
    shm_ports_coords[3].x = 0;
    shm_ports_coords[3].y = shm_cfg->SO_LATO;

    for(i = 4; i < shm_cfg->SO_PORTI; i++) {
        double rndx = (double) random() / RAND_MAX * shm_cfg->SO_LATO;
        double rndy = (double) random() / RAND_MAX * shm_cfg->SO_LATO;

        for(j = 0; j < i; j++) {
            if(shm_ports_coords[j].x == rndx && shm_ports_coords[j].y == rndy) {
                i--;
            } else {
                shm_ports_coords[i].x = rndx;
                shm_ports_coords[i].y = rndy;
            }
        }
    }
}

void create_ships(void) {
    int i;

    char *args[5];
    args[0] = int_to_string(shm_id_config);
    args[1] = int_to_string(shm_id_ports_coords);
    args[2] = int_to_string(mq_id_request);
    args[3] = int_to_string(sem_id_generation);
    args[4] = NULL;

    for(i = 0; i < shm_cfg->SO_NAVI; i++) {
        switch(ships_pid[i] = fork()) {
            case -1:
                perror("Error during: create_ships->fork()");
                raise(SIGINT);
            case 0:
                execv(PATH_NAVE, args);
                perror("execv has failed trying to run the ship");
                raise(SIGINT);
            default:
                break;
        }
    }
}

void create_ports(void) {
    int i;
    char *args[6];
    args[0] = int_to_string(shm_id_config);
    args[1] = int_to_string(shm_id_ports_coords);
    args[2] = int_to_string(mq_id_request);
    args[3] = int_to_string(sem_id_generation);
    args[5] = NULL;

    for(i = 0; i < shm_cfg->SO_PORTI; i++) {
        switch(ports_pid[i] = fork()) {
            case -1:
                perror("Error during: create_ports->fork()");
                raise(SIGINT);
            case 0:
                args[4] = int_to_string(i);
                execv(PATH_PORTO, args);
                perror("execv has failed trying to run port");
                raise(SIGINT);
            default:
                break;
        }
    }
}

void master_sig_handler(int signum) {
    int old_errno = errno;
    int i;

    switch(signum) {
        case SIGTERM:
        case SIGINT:
            printf("Error has occurred... killing all processes.\n");
            for(i = 0; i < shm_cfg->SO_NAVI; i++) {
                kill(ships_pid[i], SIGINT);
            }
            for(i = 0; i < shm_cfg->SO_PORTI; i++) {
                kill(ports_pid[i], SIGINT);
            }
            clear_all();
            exit(EXIT_FAILURE);
        /* Still needs to deal with statistics first */
        case SIGALRM:
            /* Remota possibilità di concorrenza in shm_cfg->CURRENT_DAY? */
            shm_cfg->CURRENT_DAY++;
            printf("Day [%d]/[%d].\n", shm_cfg->CURRENT_DAY, shm_cfg->SO_DAYS);

            /* Check SO_DAYS against the current day. If they're the same kill everything */
            if(shm_cfg->SO_DAYS == shm_cfg->CURRENT_DAY) {
                printf("Reached SO_DAYS, killing all processes.\n");
                for(i = 0; i < shm_cfg->SO_NAVI; i++) {
                    kill(ships_pid[i], SIGINT);
                }
                for(i = 0; i < shm_cfg->SO_PORTI; i++) {
                    kill(ports_pid[i], SIGINT);
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
        default:
            printf("Signal: %s\n", strsignal(signum));
            break;
    }
    errno = old_errno;
}
