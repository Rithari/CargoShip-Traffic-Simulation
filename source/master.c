#include "../headers/master.h"
#include "../headers/utils.h"
#include "../headers/common_ipcs.h"
#include "../headers/linked_list.h"

#define BUFFER_SIZE 128
/* initialize @*cfg with parameters wrote on @*path_cfg_file*/
void initialize_so_vars(char* path_cfg_file);
void initialize_ports_coords(void);

void create_ships(void);
void create_ports(void);
void create_weather(void);

/* clear all the memory usage */
void clear_all(void);

/* signal handler */
void master_sig_handler(int signum);

pid_t   *shm_pid_array;
pid_t   pid_weather;
config  *shm_cfg;
coord   *shm_ports_coords;

int     shm_id_config;
int     shm_id_ports_coords;
int     shm_id_pid_array;
int     mq_id_request;
int     mq_id_ships;
int     mq_id_ports;
int     sem_id_gen_precedence; /* semaphore used to manage the general precedence */
int     sem_id_docks;


int main(int argc, char **argv) {
    int i;
    struct sigaction sa;

    if(argc != 2) {
        printf("[MASTER] Incorrect number of parameters [%d]. Re-execute with: {configuration file}\n", argc);
        exit(EXIT_FAILURE);
    }

    /* create and attach config shared memory segment */
    if ((shm_id_config = shmget(IPC_PRIVATE, sizeof(*shm_cfg), 0600)) < 0) {
        perror("[MASTER] Error while creating shared memory for configuration parameters");
        exit(EXIT_FAILURE);
    }

    CHECK_ERROR((shm_cfg = shmat(shm_id_config, NULL, 0)) == (void *) -1, getpid(),
                "[MASTER] Error while trying to attach to configuration shared memory")

    initialize_so_vars(argv[1]);

    /* TODO: create goods shm*/



    /* create and attach ports coordinate shared memory segment */
    CHECK_ERROR((shm_id_ports_coords = shmget(IPC_PRIVATE,
                                              sizeof(*shm_ports_coords) * shm_cfg->SO_PORTI, 0600)) < 0, getpid(),
                "[MASTER] Error while creating shared memory for ports coordinates")

    CHECK_ERROR((shm_ports_coords = shmat(shm_id_ports_coords, NULL, 0)) == (void *) -1, getpid(),
                "[MASTER] Error while trying to attach to ports coordinates shared memory")

    initialize_ports_coords();

    /* Array of pids for the children */
    CHECK_ERROR((shm_id_pid_array = shmget(IPC_PRIVATE,
                                              sizeof(pid_t) * (shm_cfg->SO_PORTI + shm_cfg->SO_NAVI), 0600)) < 0, getpid(),
                "[MASTER] Error while creating shared memory for pid array")

    CHECK_ERROR((shm_pid_array = shmat(shm_id_pid_array, NULL, 0)) == (void *) -1, getpid(),
                "[MASTER] Error while trying to attach to pid array shared memory")

    /* create and attach message queue for ships and ports */
    CHECK_ERROR((mq_id_request = msgget(IPC_PRIVATE, 0600)) < 0, getpid(),
                "[MASTER] Error while creating message queue for requests")
    CHECK_ERROR((sem_id_gen_precedence = semget(IPC_PRIVATE, 1, 0600)) < 0, getpid(),
                "[MASTER] Error while creating semaphore for generation order control")
    CHECK_ERROR((sem_id_docks = semget(IPC_PRIVATE, shm_cfg->SO_PORTI, 0600)) < 0, getpid(),
                "[MASTER] Error while creating semaphore for docks control")

    /* Initialize sigaction struct */
    memset(&sa, 0, sizeof(sa)); /* bzero is deprecated so memset is used in its place */
    sa.sa_handler = master_sig_handler;
    sa.sa_flags = SA_RESTART; /* Restart functions if interrupted by handler */
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGCHLD, &sa, NULL);
    sigaction(SIGALRM, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGUSR1, &sa, NULL);

    /* Initialize generation semaphore at the value of SO_PORTI */
    CHECK_ERROR(semctl(sem_id_gen_precedence, 0, SETVAL, shm_cfg->SO_PORTI) < 0, getpid(),
                "[MASTER] Error while setting the semaphore for ports generation control")
    /* You're only able to generate MAX_SHORT ports here */
    create_ports();
    printf("Waiting for ports generation...\n");
    CHECK_ERROR(sem_cmd(sem_id_gen_precedence, 0, 0, 0), getpid(),
                "[MASTER] Error while waiting for ports generation")


    CHECK_ERROR(semctl(sem_id_gen_precedence, 0, SETVAL, shm_cfg->SO_NAVI) < 0, getpid(),
                "[MASTER] Error while setting the semaphore for ships generation control")
    create_ships();
    printf("Waiting for ships generation...\n");
    CHECK_ERROR(sem_cmd(sem_id_gen_precedence, 0, 0, 0), getpid(),
                "[MASTER] Error while waiting for ships generation")

    create_weather();
    /*TODO: utilizzo SIGALRM per sincronizzare tutti i processi per iniziare il primo giorno,
     * credo sia meglio implementare ad esempio una SIGCONT*/
    /*TODO: magari implementare setpgid, getpgid, setpgrp, getpgrp*/

    /*for(i = 0; i < shm_cfg->SO_PORTI + shm_cfg->SO_NAVI; i++) {
        if(shm_pid_array[i] >= 0) {
            kill(shm_pid_array[i], SIGCONT);
        }
    }*/

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
    printf("FINE SIMULAZIONE!\n\n");
    return 0;
}

void clear_all(void) {
    pid_t pid;
    pid = getpid();

    CHECK_ERROR(shmctl(shm_id_config, IPC_RMID, NULL), pid,
                "[MASTER] Error while removing config shared memory in clear_all");
    CHECK_ERROR(shmctl(shm_id_ports_coords, IPC_RMID, NULL), pid,
                "[MASTER] Error while removing ports coordinates shared memory in clear_all");
    CHECK_ERROR(shmctl(shm_id_pid_array, IPC_RMID, NULL), pid,
                "[MASTER] Error while removing pid array shared memory in clear_all");
    CHECK_ERROR(msgctl(mq_id_request, IPC_RMID, NULL), pid,
                "[MASTER] Error while removing message queue in clear_all");
    CHECK_ERROR(semctl(sem_id_gen_precedence, 0, IPC_RMID, 0), pid,
                "[MASTER] Error while removing semaphore for generation order control in clear_all");
    CHECK_ERROR(semctl(sem_id_docks, 0, IPC_RMID, 0), pid,
                "[MASTER] Error while removing semaphore for docks control in clear_all");
}

void initialize_so_vars(char* path_cfg_file) {
    /* Configuration file setup */
    FILE *fp;
    char buffer[BUFFER_SIZE];

    fp = fopen(path_cfg_file, "r");
    if(!fp) {
        perror("Error during: initialize_so_vars->fopen()");
        raise(SIGINT);
    }

    shm_cfg->check = 0;

    while (!feof(fp)) {
        if(fgets(buffer, BUFFER_SIZE, fp) == NULL) {
            break;
        }

        if(sscanf(buffer, "#%s", buffer) == 1) {
            continue;
        }

        if (sscanf(buffer, "SO_NAVI: %d", &shm_cfg->SO_NAVI) == 1) {
            shm_cfg->check |= 1;
        } else if (sscanf(buffer, "SO_PORTI: %d", &shm_cfg->SO_PORTI) == 1) {
            shm_cfg->check |= 1 << 1;
        } else if (sscanf(buffer, "SO_MERCI: %d", &shm_cfg->SO_MERCI) == 1) {
            shm_cfg->check |= 1 << 2;
        } else if (sscanf(buffer, "SO_SIZE: %d", &shm_cfg->SO_SIZE) == 1) {
            shm_cfg->check |= 1 << 3;
        } else if (sscanf(buffer, "SO_MIN_VITA: %d", &shm_cfg->SO_MIN_VITA) == 1) {
            shm_cfg->check |= 1 << 4;
        } else if (sscanf(buffer, "SO_MAX_VITA: %d", &shm_cfg->SO_MAX_VITA) == 1) {
            shm_cfg->check |= 1 << 5;
        } else if (sscanf(buffer, "SO_LATO: %lf", &shm_cfg->SO_LATO) == 1) {
            shm_cfg->check |= 1 << 6;
        } else if (sscanf(buffer, "SO_SPEED: %d", &shm_cfg->SO_SPEED) == 1) {
            shm_cfg->check |= 1 << 7;
        } else if (sscanf(buffer, "SO_CAPACITY: %d", &shm_cfg->SO_CAPACITY) == 1) {
            shm_cfg->check |= 1 << 8;
        } else if (sscanf(buffer, "SO_BANCHINE: %d", &shm_cfg->SO_BANCHINE) == 1) {
            shm_cfg->check |= 1 << 9;
        } else if (sscanf(buffer, "SO_FILL: %d", &shm_cfg->SO_FILL) == 1) {
            shm_cfg->check |= 1 << 10;
        } else if (sscanf(buffer, "SO_LOADSPEED: %d", &shm_cfg->SO_LOADSPEED) == 1) {
            shm_cfg->check |= 1 << 11;
        } else if (sscanf(buffer, "SO_DAYS: %d", &shm_cfg->SO_DAYS) == 1) {
            shm_cfg->check |= 1 << 12;
        } else if (sscanf(buffer, "SO_DAY_LENGTH: %d", &shm_cfg->SO_DAY_LENGTH) == 1) {
            shm_cfg->check |= 1 << 13;
        } else if (sscanf(buffer, "SO_STORM_DURATION: %d", &shm_cfg->SO_STORM_DURATION) == 1) {
            shm_cfg->check |= 1 << 14;
        } else if (sscanf(buffer, "SO_SWELL_DURATION: %d", &shm_cfg->SO_SWELL_DURATION) == 1) {
            shm_cfg->check |= 1 << 15;
        } else if (sscanf(buffer, "SO_MAELSTROM: %d", &shm_cfg->SO_MAELSTROM) == 1) {
            shm_cfg->check |= 1 << 16;
        }
    }
    fclose(fp);

    shm_cfg->CURRENT_DAY = 0;

    /* A "reverse" if statement */
    errno = EINVAL;

    CHECK_ERROR(shm_cfg->check != 0x1FFFF, getpid(), "Missing config")
    CHECK_ERROR(shm_cfg->SO_NAVI < 1, getpid(), "SO_NAVI is less than 1")
    CHECK_ERROR(shm_cfg->SO_PORTI < 4, getpid(), "SO_PORTI is less than 4")
    CHECK_ERROR(shm_cfg->SO_PORTI > SHRT_MAX, getpid(),
                "SO_PORTI is greater than SHRT_MAX, so we cannot use semaphores for ensure correct generation order.")
    CHECK_ERROR(shm_cfg->SO_MIN_VITA > shm_cfg->SO_MAX_VITA, getpid(), "SO_MIN_VITA is greater than SO_MAX_VITA")
    CHECK_ERROR(shm_cfg->SO_SPEED <= 0, getpid(), "SO_SPEED is less or equal than 0")
    CHECK_ERROR(shm_cfg->SO_BANCHINE > SHRT_MAX, getpid(),
                "SO_BANCHINE is greater than SHRT_MAX, so we cannot use semaphores for ensure correct docks management.")
    errno = 0;
}

void initialize_ports_coords(void) {
    int i, j;

    /* Hardcode first 4 port coordinates and start the loop at 4 for random coordinates */
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
                break;
            }
        }
    }
}

/* TODO: error checking */
void create_ports(void) {
    /* Pass the shared memory information through as arguments to the child processes */
    int i;
    char *args[8];
    pid_t pid_process;

    args[0] = PATH_PORTO;
    args[1] = int_to_string(shm_id_config);
    args[2] = int_to_string(shm_id_ports_coords);
    args[3] = int_to_string(mq_id_request);
    args[4] = int_to_string(sem_id_gen_precedence);
    args[5] = int_to_string(sem_id_docks);
    args[7] = NULL;

    for(i = 0; i < shm_cfg->SO_PORTI; i++) {
        switch(pid_process = fork()) {
            case -1:
                perror("[MASTER] Error during: create_ports->fork()");
                raise(SIGINT);
            case 0:
                args[6] = int_to_string(i);
                CHECK_ERROR(semctl(sem_id_docks, i, SETVAL, random() % shm_cfg->SO_BANCHINE + 1), getpid(),
                            "[PORTO] Error while generating dock semaphore")
                execv(PATH_PORTO, args);
                perror("[PORTO] execv has failed trying to run port");
                kill(getppid(), SIGINT);
            default:
                shm_pid_array[i] = pid_process;
                break;
        }
    }

    /* Clear up the memory allocated for the arguments */
    for(i = 1; i < 6; i++) {
        free(args[i]);
    }
}

void create_ships(void) {
    /* Pass the shared memory information through as arguments to the child processes */
    int i;
    char *args[7];
    pid_t pid_process;

    args[0] = PATH_NAVE;
    args[1] = int_to_string(shm_id_config);
    args[2] = int_to_string(shm_id_ports_coords);
    args[3] = int_to_string(mq_id_request);
    args[4] = int_to_string(sem_id_gen_precedence);
    args[5] = int_to_string(sem_id_docks);
    args[6] = NULL;

    for(i = 0; i < shm_cfg->SO_NAVI; i++) {
        switch(pid_process = fork()) {
            case -1:
                perror("[MASTER] Error during: create_ships->fork()");
                raise(SIGINT);
            case 0:
                execv(PATH_NAVE, args);
                perror("[NAVE] execv has failed trying to run the ship");
                kill(getppid(), SIGINT);
            default:
                shm_pid_array[i + shm_cfg->SO_PORTI] = pid_process;
                break;
        }
    }

    /* Clear up the memory allocated for the arguments */
    for(i = 1; i < 6; i++) {
        free(args[i]);
    }
}

void create_weather(void) {
    /* Pass the shared memory information through as arguments to the child processes */
    int i;
    char *args[4];

    args[0] = PATH_METEO;
    args[1] = int_to_string(shm_id_config);
    args[2] = int_to_string(shm_id_pid_array);
    args[3] = NULL;

    switch(pid_weather = fork()) {
        case -1:
            perror("Error during: create_weather->fork()");
            raise(SIGINT);
        case 0:
            execv(PATH_METEO, args);
            perror("[METEO] execv has failed trying to run the weather process");
            kill(getppid(), SIGINT);
        default:
            break;
    }

    /* Clear up the memory allocated for the arguments */
    for (i = 1; i < 3; i++) {
        free(args[i]);
    }
}

void master_sig_handler(int signum) {
    int old_errno = errno;
    int i;

    /*TODO: migliore implementazione di tutti i segnali. Schematizzazione necessaria*/
    switch(signum) {
        case SIGTERM:
        case SIGINT:
            printf("Error has occurred... killing all processes.\n");
            for(i = 0; i < shm_cfg->SO_PORTI + shm_cfg->SO_NAVI; i++) {
                if(shm_pid_array[i] >= 0) {
                    kill(shm_pid_array[i], SIGINT);
                }
            }
            kill(pid_weather, SIGINT);
            clear_all();
            exit(EXIT_FAILURE);
        /* Still needs to deal with statistics first */
        case SIGALRM:
            shm_cfg->CURRENT_DAY++;

            /* Check SO_DAYS against the current day. If they're the same kill everything */
            if(shm_cfg->CURRENT_DAY == shm_cfg->SO_DAYS) {
                printf("Reached SO_DAYS, killing all processes.\n");
                for(i = 0; i < shm_cfg->SO_PORTI + shm_cfg->SO_NAVI; i++) {
                    if(shm_pid_array[i] >= 0) {
                        kill(shm_pid_array[i], SIGINT);
                    }
                }
                kill(pid_weather, SIGINT);
            } else {
                printf("Day [%d]/[%d].\n", shm_cfg->CURRENT_DAY, shm_cfg->SO_DAYS);
                for(i = 0; i < shm_cfg->SO_PORTI + shm_cfg->SO_NAVI; i++) {
                    if(shm_pid_array[i] >= 0) {
                        kill(shm_pid_array[i], SIGALRM);
                    }
                }
                kill(pid_weather, SIGALRM);
            }
            alarm(shm_cfg->SO_DAY_LENGTH);
            break;
        case SIGCHLD:
            break;
        case SIGUSR1:
            printf("ALL SHIPS ARE DEAD :C\n");
            for(i = 0; i < shm_cfg->SO_PORTI; i++) {
                if(shm_pid_array[i] >= 0) {
                    kill(shm_pid_array[i], SIGTERM);
                }
            }
            break;
        default:
            printf("Signal: %s\n", strsignal(signum));
            break;
    }
    errno = old_errno;
}
