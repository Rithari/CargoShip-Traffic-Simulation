#include "../headers/master.h"
#include "../headers/utils.h"
#include "../headers/common_ipcs.h"

#define BUFFER_SIZE 128

/* Macro for error checking an expression. If it evaluates to true the error is printed and the process exits */
#define CHECK_ERROR_MASTER(x, str)  if ((x)) { \
                                        perror(str); \
                                        raise(SIGINT); \
                                    } else {}\

/* Initialize @*cfg with parameters written on @*path_cfg_file*/
void initialize_so_vars(char* path_cfg_file);
void initialize_ports_coords(void);

void create_ships(void);
void create_ports(void);
void create_weather(void);

void clear_all(void); /* clear all the memory usage by destroying IPC objects */
void print_dump(void);
void generate_goods(void);

void master_sig_handler(int signum);

pid_t   *shm_pid_array;
pid_t   pid_weather;
config  *shm_cfg;
coord   *shm_ports_coords;
goods_template   *shm_goods_template;
int     *shm_goods;
dump_ports  *shm_dump_ports;
dump_ships  *shm_dump_ships;
dump_goods  *shm_dump_goods;
FILE *output;
int     shm_id_config;

int main(int argc, char **argv) {
    int i;
    int wstatus;
    pid_t p_wait;
    struct sigaction sa;

    if(argc != 2) {
        printf("[MASTER] Incorrect number of parameters [%d]. Re-execute with: {configuration file}\n", argc);
        exit(EXIT_FAILURE);
    }

    /* Make sure clear all is called when master exits to ensure proper IPC removal */
    CHECK_ERROR_MASTER(atexit(clear_all), "[MASTER] Error while trying to register clear_all function to atexit")

    /* Check if the file path in argv1 exists */
    CHECK_ERROR_MASTER(access(argv[1], F_OK), "[MASTER] Error while trying to access the configuration file")

    /* Initialize random seed */
    srandom(getpid());

    /* CREATE AND ATTACH TO SHARED MEMORY
     * After every few IPC object creations, a function may be called, which typically remains self-explanatory
     * and is, if needed, further detailed in its implementation */
    if ((shm_id_config = shmget(IPC_PRIVATE, sizeof(*shm_cfg), 0600)) < 0) {
        perror("[MASTER] Error while creating shared memory for configuration parameters");
        exit(EXIT_FAILURE);
    }
    CHECK_ERROR_MASTER((shm_cfg = shmat(shm_id_config, NULL, 0)) == (void *) -1,
                "[MASTER] Error while trying to attach to configuration shared memory")

    /* Initialize the configuration parameters */
    initialize_so_vars(argv[1]);

    CHECK_ERROR_MASTER((shm_cfg->shm_id_goods_template = shmget(IPC_PRIVATE,
                                                                sizeof(goods) * shm_cfg->SO_MERCI, 0600)) < 0,
                "[MASTER] Error while creating shared memory for goods_template generation")
    CHECK_ERROR_MASTER((shm_goods_template = shmat(shm_cfg->shm_id_goods_template, NULL, 0)) == (void *) -1,
                "[MASTER] Error while trying to attach to goods template shared memory")

    /* Initialize the goods_template array */
    for (i = 0; i < shm_cfg->SO_MERCI; i++) {
        shm_goods_template[i].tons = (int) random() % shm_cfg->SO_SIZE + 1;
        shm_goods_template[i].lifespan = (int) random() % (shm_cfg->SO_MAX_VITA - shm_cfg->SO_MIN_VITA + 1)
                                         + shm_cfg->SO_MIN_VITA;
    }
    /* Quicksort based on the structs tons field (using compare_goods_template) */
    qsort(shm_goods_template, shm_cfg->SO_MERCI, sizeof(goods_template), compare_goods_template);

    /* Also save the port coordinates in shared memory */
    CHECK_ERROR_MASTER((shm_cfg->shm_id_ports_coords = shmget(IPC_PRIVATE,
                                              sizeof(*shm_ports_coords) * shm_cfg->SO_PORTI, 0600)) < 0,
                "[MASTER] Error while creating shared memory for ports coordinates")
    CHECK_ERROR_MASTER((shm_ports_coords = shmat(shm_cfg->shm_id_ports_coords, NULL, 0)) == (void *) -1,
                "[MASTER] Error while trying to attach to ports coordinates shared memory")

    /* Initialize the ports coordinates array */
    initialize_ports_coords();

    /* Create and attach the PID array of the children (which will be populated in the subsequent forks) */
    CHECK_ERROR_MASTER((shm_cfg->shm_id_pid_array =
            shmget(IPC_PRIVATE, sizeof(pid_t) * (shm_cfg->SO_PORTI + shm_cfg->SO_NAVI), 0600)) < 0,
                "[MASTER] Error while creating pid array shared memory")
    CHECK_ERROR_MASTER((shm_pid_array = shmat(shm_cfg->shm_id_pid_array, NULL, 0)) == (void *) -1,
                "[MASTER] Error while trying to attach to pid array shared memory")


    /* Create and attach the PID array of goods tracked as offers */
    CHECK_ERROR_MASTER((shm_cfg->shm_id_goods =
                                shmget(IPC_PRIVATE, sizeof(int) * shm_cfg->SO_PORTI * shm_cfg->SO_MERCI, 0600)) < 0,
                       "[MASTER] Error while creating goods_tracking shared memory")
    CHECK_ERROR_MASTER((shm_goods = shmat(shm_cfg->shm_id_goods, NULL, 0)) == (void *) -1,
                       "[MASTER] Error while trying to attach to goods_tracking shared memory")

    /* Initialize the goods array at 0 */
    memset(shm_goods, 0, sizeof(int) * shm_cfg->SO_PORTI * shm_cfg->SO_MERCI);

    /* Create and attach the PID array of dump reports */
    CHECK_ERROR_MASTER((shm_cfg->shm_id_dump_ports = shmget(IPC_PRIVATE, sizeof(dump_ports) * shm_cfg->SO_PORTI, 0600)) < 0,
                "[MASTER] Error while creating shared memory for log ports data")
    CHECK_ERROR_MASTER((shm_dump_ports = shmat(shm_cfg->shm_id_dump_ports, NULL, 0)) == (void*) -1,
                "[MASTER] Error while trying to attach to log ports data shared memory")

    CHECK_ERROR_MASTER((shm_cfg->shm_id_dump_ships = shmget(IPC_PRIVATE, sizeof(dump_ships), 0600)) < 0,
                "[MASTER] Error while creating shared memory for log ships data")
    CHECK_ERROR_MASTER((shm_dump_ships = shmat(shm_cfg->shm_id_dump_ships, NULL, 0)) == (void*) -1,
                "[MASTER] Error while trying to attach to log ships data shared memory")

    CHECK_ERROR_MASTER((shm_cfg->shm_id_dump_goods = shmget(IPC_PRIVATE, sizeof(dump_goods) * shm_cfg->SO_MERCI, 0600)) < 0,
                "[MASTER] Error while creating shared memory for log goods_template data")
    CHECK_ERROR_MASTER((shm_dump_goods = shmat(shm_cfg->shm_id_dump_goods, NULL, 0)) == (void*) -1,
                "[MASTER] Error while trying to attach to log goods_template data shared memory")


    /* Deletes all the contents of shared memory arrays */
    memset(shm_dump_ports, 0, sizeof(dump_ports) * shm_cfg->SO_PORTI);
    memset(shm_dump_ships, 0, sizeof(dump_ships));
    memset(shm_dump_goods, 0, sizeof(dump_goods) * shm_cfg->SO_MERCI);

    /* Creates the messages queues for the handshake between ports and ships */
    CHECK_ERROR_MASTER((shm_cfg->mq_id_ports_handshake = msgget(IPC_PRIVATE, 0600)) < 0,
                "[MASTER] Error while creating ports_handshake message queue")
    CHECK_ERROR_MASTER((shm_cfg->mq_id_ships_handshake = msgget(IPC_PRIVATE, 0600)) < 0,
                       "[MASTER] Error while creating ships_handshake message queue")
    CHECK_ERROR_MASTER((shm_cfg->mq_id_ships_goods = msgget(IPC_PRIVATE, 0600)) < 0,
                       "[MASTER] Error while creating ships_goods message queue")

    /* Create the semaphore for process generation control and the dump status */
    CHECK_ERROR_MASTER((shm_cfg->sem_id_gen_precedence = semget(IPC_PRIVATE, 1, 0600)) < 0,
                "[MASTER] Error while creating semaphore for generation order control")

    /* Create the semaphore for dock generation control and the dump status */
    CHECK_ERROR_MASTER((shm_cfg->sem_id_dock = semget(IPC_PRIVATE, shm_cfg->SO_PORTI, 0600)) < 0,
                       "[MASTER] Error while creating semaphore for docks control")

    /* TODO: Look into this */
    /* WIP: create the semaphore for process generation control and the dump status, 7 is a test number */
    CHECK_ERROR_MASTER((shm_cfg->sem_id_dump_mutex = semget(IPC_PRIVATE, 7, 0600)) < 0,
                "[MASTER] Error while creating semaphore for dump control")

    /* i = 0 -> semaforo utilizzato per sincronizzare i dump della nave */
    /* i = 1 -> semaforo utilizzato per sincronizzare i dump della storm */
    for (i = 0; i < 7; i++) {
        CHECK_ERROR_MASTER(semctl(shm_cfg->sem_id_dump_mutex, i, SETVAL, 1) < 0,
                    "[MASTER] Error while setting the semaphore for ports dump control")
    }

    /* Write dumps to output.txt */
    output = fopen("output.txt", "w");

    /* Initialize generation semaphore at the value of SO_PORTI */
    CHECK_ERROR_MASTER(semctl(shm_cfg->sem_id_gen_precedence, 0, SETVAL, shm_cfg->SO_PORTI),
                "[MASTER] Error while setting the semaphore for ports generation control")

    create_ports();

    printf("Waiting for ports generation...\n");
    CHECK_ERROR_MASTER(sem_cmd(shm_cfg->sem_id_gen_precedence, 0, 0, 0),
                "[MASTER] Error while waiting for ports generation")

    /* generates day 0 goods */
    generate_goods();
    CHECK_ERROR_MASTER(semctl(shm_cfg->sem_id_gen_precedence, 0, SETVAL, shm_cfg->SO_NAVI),
                "[MASTER] Error while setting the semaphore for ships generation control")
    create_ships();
    printf("Waiting for ships generation...\n");
    CHECK_ERROR_MASTER(sem_cmd(shm_cfg->sem_id_gen_precedence, 0, 0, 0),
                "[MASTER] Error while waiting for ships generation")

    create_weather();

    /* Initialize sigaction struct */
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = master_sig_handler;
    sa.sa_flags = SA_RESTART;
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGALRM, &sa, NULL);
    sigaction(SIGUSR1, &sa, NULL);

    printf("SIMULATION START\n");
    killpg(0, SIGCONT); /* Send SIGCONT to all processes in the group (foreground process group)*/

    /* Day 1 Alarm */
    alarm(shm_cfg->SO_DAY_LENGTH);

    /* Handle any extra signals, especially if they're the kind that terminates a process */
    while((p_wait = waitpid(-1, &wstatus, 0)) > 0) {
        if (WIFEXITED(wstatus) && WEXITSTATUS(wstatus) != 0) {
            printf("[MASTER] Child [%d] exit with status: %d. Quitting...\n", p_wait, WEXITSTATUS(wstatus));
            raise(SIGINT);
        }

        if(WIFSIGNALED(wstatus)) {
            printf("[MASTER] CHILD [%d] GOT SIGNAL: %s\n", p_wait, strsignal(WTERMSIG(wstatus)));
            raise(SIGINT);
        }
    }

    print_dump(); /* final dump */
    printf("SIMULATION ENDED\n\n");
    fclose(output);

    return 0;
}

void clear_all(void) {
    CHECK_ERROR_MASTER(shmctl(shm_id_config, IPC_RMID, NULL),
                       "[MASTER] Error while removing config shared memory in clear_all")
    CHECK_ERROR_MASTER(shmctl(shm_cfg->shm_id_goods_template, IPC_RMID, NULL),
                       "[MASTER] Error while removing goods_template template shared memory in clear_all")
    CHECK_ERROR_MASTER(shmctl(shm_cfg->shm_id_ports_coords, IPC_RMID, NULL),
                       "[MASTER] Error while removing ports coordinates shared memory in clear_all")
    CHECK_ERROR_MASTER(shmctl(shm_cfg->shm_id_pid_array, IPC_RMID, NULL),
                       "[MASTER] Error while removing pid array shared memory in clear_all")
    CHECK_ERROR_MASTER(shmctl(shm_cfg->shm_id_goods, IPC_RMID, NULL),
                       "[MASTER] Error while removing pid array shared memory in clear_all")
    CHECK_ERROR_MASTER(shmctl(shm_cfg->shm_id_dump_ports, IPC_RMID, NULL),
                       "[MASTER] Error while removing dump simulation shared memory in clear_all")
    CHECK_ERROR_MASTER(shmctl(shm_cfg->shm_id_dump_ships, IPC_RMID, NULL),
                       "[MASTER] Error while removing dump simulation shared memory in clear_all")
    CHECK_ERROR_MASTER(shmctl(shm_cfg->shm_id_dump_goods, IPC_RMID, NULL),
                       "[MASTER] Error while removing dump simulation shared memory in clear_all")
    CHECK_ERROR_MASTER(msgctl(shm_cfg->mq_id_ports_handshake, IPC_RMID, NULL),
                "[MASTER] Error while removing mq_id_ports_handshake message queue in clear_all")
    CHECK_ERROR_MASTER(msgctl(shm_cfg->mq_id_ships_handshake, IPC_RMID, NULL),
                "[MASTER] Error while removing mq_id_ports_handshake message queue in clear_all")
    CHECK_ERROR_MASTER(msgctl(shm_cfg->mq_id_ships_goods, IPC_RMID, NULL),
                       "[MASTER] Error while removing mq_id_ports_handshake message queue in clear_all")
    CHECK_ERROR_MASTER(semctl(shm_cfg->sem_id_gen_precedence, 0, IPC_RMID, 0),
                "[MASTER] Error while removing semaphore for generation order control in clear_all")
    CHECK_ERROR_MASTER(semctl(shm_cfg->sem_id_dock, 0, IPC_RMID, 0),
                "[MASTER] Error while removing semaphore for docks control in clear_all")
    CHECK_ERROR_MASTER(semctl(shm_cfg->sem_id_dump_mutex, 0, IPC_RMID, 0),
                "[MASTER] Error while removing semaphore for dump control in clear_all")
}

void initialize_so_vars(char* path_cfg_file) {
    FILE *fp;
    char buffer[BUFFER_SIZE];
    int check = 0;

    CHECK_ERROR_MASTER(!(fp = fopen(path_cfg_file, "r")), "Error during: initialize_so_vars->fopen()")

    while (!feof(fp)) {
        if(fgets(buffer, BUFFER_SIZE, fp) == NULL) {
            break;
        }

        if(sscanf(buffer, "#%s", buffer) == 1 || buffer[0] == '\n') {
            continue;
        }

        /* TODO: Explain why sscanf is used and what the bitwise operations do, in depth */
        if (sscanf(buffer, "SO_NAVI: %d", &shm_cfg->SO_NAVI) == 1) {
            check |= 1;
        } else if (sscanf(buffer, "SO_PORTI: %d", &shm_cfg->SO_PORTI) == 1) {
            check |= 1 << 1;
        } else if (sscanf(buffer, "SO_MERCI: %d", &shm_cfg->SO_MERCI) == 1) {
            check |= 1 << 2;
        } else if (sscanf(buffer, "SO_SIZE: %d", &shm_cfg->SO_SIZE) == 1) {
            check |= 1 << 3;
        } else if (sscanf(buffer, "SO_MIN_VITA: %d", &shm_cfg->SO_MIN_VITA) == 1) {
            check |= 1 << 4;
        } else if (sscanf(buffer, "SO_MAX_VITA: %d", &shm_cfg->SO_MAX_VITA) == 1) {
            check |= 1 << 5;
        } else if (sscanf(buffer, "SO_LATO: %lf", &shm_cfg->SO_LATO) == 1) {
            check |= 1 << 6;
        } else if (sscanf(buffer, "SO_SPEED: %d", &shm_cfg->SO_SPEED) == 1) {
            check |= 1 << 7;
        } else if (sscanf(buffer, "SO_CAPACITY: %d", &shm_cfg->SO_CAPACITY) == 1) {
            check |= 1 << 8;
        } else if (sscanf(buffer, "SO_BANCHINE: %d", &shm_cfg->SO_BANCHINE) == 1) {
            check |= 1 << 9;
        } else if (sscanf(buffer, "SO_FILL: %d", &shm_cfg->SO_FILL) == 1) {
            check |= 1 << 10;
        } else if (sscanf(buffer, "SO_LOADSPEED: %d", &shm_cfg->SO_LOADSPEED) == 1) {
            check |= 1 << 11;
        } else if (sscanf(buffer, "SO_DAYS: %d", &shm_cfg->SO_DAYS) == 1) {
            check |= 1 << 12;
        } else if (sscanf(buffer, "SO_DAY_LENGTH: %d", &shm_cfg->SO_DAY_LENGTH) == 1) {
            check |= 1 << 13;
        } else if (sscanf(buffer, "SO_STORM_DURATION: %d", &shm_cfg->SO_STORM_DURATION) == 1) {
            check |= 1 << 14;
        } else if (sscanf(buffer, "SO_SWELL_DURATION: %d", &shm_cfg->SO_SWELL_DURATION) == 1) {
            check |= 1 << 15;
        } else if (sscanf(buffer, "SO_MAELSTORM: %d", &shm_cfg->SO_MAELSTORM) == 1) {
            check |= 1 << 16;
        }
    }
    CHECK_ERROR_MASTER(fclose(fp), "[MASTER] Error while closing the fp")

    shm_cfg->CURRENT_DAY = 0;

    /* Checks the validity of the variables. By setting EINVAL, if an error occurs we know it's due to invalid args */
    errno = EINVAL;
    CHECK_ERROR_MASTER(check != 0x1FFFF, "[MASTER] Missing config")
    CHECK_ERROR_MASTER(shm_cfg->SO_NAVI < 1, "[MASTER] SO_NAVI is less than 1")
    CHECK_ERROR_MASTER(shm_cfg->SO_PORTI < 4, "[MASTER] SO_PORTI is less than 4")
    CHECK_ERROR_MASTER(shm_cfg->SO_PORTI + shm_cfg->SO_NAVI > USHRT_MAX,
                       "[MASTER] Too many processes than the sem_val limit")
    CHECK_ERROR_MASTER(shm_cfg->SO_PORTI + shm_cfg->SO_NAVI + 1 > sysconf(_SC_CHILD_MAX),
                       "[MASTER] Too many processes than the _SC_CHILD_MAX limit")
    CHECK_ERROR_MASTER(shm_cfg->SO_MERCI <= 0, "[MASTER] SO_MERCI is less or equal than 0")
    CHECK_ERROR_MASTER(shm_cfg->SO_MIN_VITA <= 0, "[MASTER] SO_MIN_VITA is less or equal than 0")
    CHECK_ERROR_MASTER(shm_cfg->SO_MIN_VITA > shm_cfg->SO_MAX_VITA, "[MASTER] SO_MIN_VITA is greater than SO_MAX_VITA")
    CHECK_ERROR_MASTER(shm_cfg->SO_LATO <= 0, "[MASTER] SO_LATO is less or equal than 0")
    CHECK_ERROR_MASTER(shm_cfg->SO_SPEED <= 0, "[MASTER] SO_SPEED is less or equal than 0")
    CHECK_ERROR_MASTER(shm_cfg->SO_CAPACITY <= 0, "[MASTER] SO_CAPACITY is less or equal than 0")
    CHECK_ERROR_MASTER(shm_cfg->SO_BANCHINE <= 0, "[MASTER] SO_BANCHINE is less or equal than 0")
    CHECK_ERROR_MASTER(shm_cfg->SO_BANCHINE > USHRT_MAX, "SO_BANCHINE is greater than the sem_val limit")
    CHECK_ERROR_MASTER(shm_cfg->SO_FILL <= 0, "[MASTER] SO_FILL is less or equal than 0")
    CHECK_ERROR_MASTER(shm_cfg->SO_LOADSPEED <= 0, "[MASTER] SO_LOADSPEED is less or equal than 0")
    CHECK_ERROR_MASTER(shm_cfg->SO_DAYS <= 0, "[MASTER] SO_DAYS is less or equal than 0")
    CHECK_ERROR_MASTER(shm_cfg->SO_DAY_LENGTH <= 0, "[MASTER] SO_DAY_LENGTH is less or equal than 0")
    CHECK_ERROR_MASTER(shm_cfg->SO_STORM_DURATION < 0, "[MASTER] SO_STORM_DURATION is less than 0")
    CHECK_ERROR_MASTER(shm_cfg->SO_SWELL_DURATION < 0, "[MASTER] SO_SWELL_DURATION is less than 0")
    CHECK_ERROR_MASTER(shm_cfg->SO_MAELSTORM < 0, "[MASTER] SO_MAELSTORM is less than 0")
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

        /* Check if the coordinates are already used */
        for(j = 0; j < i && !(shm_ports_coords[j].x == rndx && shm_ports_coords[j].y == rndy); j++);

        if(j == i) {
            shm_ports_coords[i].x = rndx;
            shm_ports_coords[i].y = rndy;
        } else i--;
    }
}

void create_ports(void) {
    /* Pass the shared memory information through as arguments to the child processes */
    int i;
    char *args[4];
    pid_t pid_process;

    args[0] = PATH_PORTO;
    CHECK_ERROR_MASTER(asprintf(&args[1], "%d", shm_id_config) < 0,
                "[MASTER] Error while converting shm_id_config into a string")
    args[3] = NULL;

    for(i = 0; i < shm_cfg->SO_PORTI; i++) {
        switch(pid_process = fork()) {
            case -1:
                perror("[MASTER] Error during: create_ports->fork()");
                raise(SIGINT);
                break;
            case 0:
                CHECK_ERROR_CHILD(asprintf(&args[2], "%d", i) < 0,
                            "[PORTO] Error while converting index into a string")
                CHECK_ERROR_CHILD(execv(PATH_PORTO, args), "[PORTO] execv has failed trying to run port")
                break;
            default:
                shm_pid_array[i] = pid_process;
                break;
        }
    }
    /* Clear up the memory allocated by asprintf for the first argument */
    free(args[1]);
}

void create_ships(void) {
    /* Pass the shared memory information through as arguments to the child processes */
    int i;
    char *args[4];
    pid_t pid_process;

    args[0] = PATH_NAVE;
    CHECK_ERROR_MASTER(asprintf(&args[1], "%d", shm_id_config) < 0,
                "[MASTER] Error while converting shm_id_config into a string")
    args[3] = NULL;

    for(i = 0; i < shm_cfg->SO_NAVI; i++) {
        switch(pid_process = fork()) {
            case -1:
                perror("[MASTER] Error during: create_ships->fork()");
                raise(SIGINT);
                break;
            case 0:
                CHECK_ERROR_MASTER(asprintf(&args[2], "%d", i) < 0,
                                   "[NAVE] Error while converting index into a string")
                CHECK_ERROR_CHILD(execv(PATH_NAVE, args), "[NAVE] execv has failed trying to run the ship")
                break;
            default:
                shm_pid_array[i + shm_cfg->SO_PORTI] = pid_process;
                break;
        }
    }

    /* Clear up the memory allocated by asprintf for the first argument */
    free(args[1]);
}

void create_weather(void) {
    /* Pass the shared memory information through as arguments to the child processes */
    char *args[3];

    args[0] = PATH_METEO;
    CHECK_ERROR_MASTER(asprintf(&args[1], "%d", shm_id_config) < 0,
                "[MASTER] Error while converting shm_id_config into a string")
    args[2] = NULL;

    switch(pid_weather = fork()) {
        case -1:
            perror("Error during: create_weather->fork()");
            raise(SIGINT);
            break;
        case 0:
            CHECK_ERROR_CHILD(execv(PATH_METEO, args), "[METEO] execv has failed trying to run the weather process")
        default:
            break;
    }

    /* Clear up the memory allocated by asprintf for the first argument */
    free(args[1]);
}

/* TODO: pull from updated dumps */
void print_dump(void) {
    int i;
    fprintf(output, "----------------------\n");
    /*printf("---MERCI---\n");
    for(i = 0; i < shm_cfg->SO_MERCI; i++) {
        printf("ID: [%d]\tSTATE: [%d]\n", shm_dump_goods[i].id, shm_dump_goods[i].state);
        printf("------\n");
    }*/
    fprintf(output,"---PORTI---\n");
    for(i = 0; i < shm_cfg->SO_PORTI; i++) {
        fprintf(output,"ID: [%d]\tON_SWELL: [%d]\n", i, shm_dump_ports[i].on_swell);
        fprintf(output,"DOCK: [%d/%d]\n", shm_dump_ports[i].dock_available, shm_dump_ports[i].dock_total);
        fprintf(output,"GOODS: [%d/%d/%d/%d]\n", shm_dump_ports[i].good_available, shm_dump_ports[i].good_send,
               shm_dump_ports[i].good_received, shm_dump_ports[i].ton_in_excess);
        fprintf(output,"------\n");
    }
    fprintf(output,"---NAVI---\n");
    fprintf(output,"SHIPS: [%d/%d/%d]\n", shm_dump_ships->with_cargo_en_route,
           shm_dump_ships->without_cargo_en_route, shm_dump_ships->being_loaded_unloaded);
    fprintf(output,"SHIPS SLOWED: [%d]\tSHIPS SUNK: [%d]\n", shm_dump_ships->slowed, shm_dump_ships->sunk);
    fprintf(output,"----------------------\n");
}

void generate_goods(void) {
    int i;
    shm_cfg->GENERATING_PORTS = (int) random() % shm_cfg->SO_PORTI;
    /* this variable indicates the randomly chosen number of ports that will be marked to generate goods on this day*/

    /* Go through the chosen amount of ports and mark a random index to be generating goods */
    for (i = 0; i < shm_cfg->GENERATING_PORTS; i++) {
        int index = (int) random() % shm_cfg->SO_PORTI;
        shm_pid_array[index] = -shm_pid_array[index]; /* This is a flag to indicate that this port is generating goods */
    }

    printf("[MASTER] GENERATING_PORTS: [%d]\n", shm_cfg->GENERATING_PORTS);
}

void master_sig_handler(int signum) {
    int old_errno = errno; /* Save errno to restore it after the signal handler returns */
    int i, check_port_offers, check_port_request;

    switch(signum) {
        case SIGTERM:
        case SIGINT:
            printf("Error has occurred... killing all processes.\n");
            for(i = 0; i < shm_cfg->SO_PORTI + shm_cfg->SO_NAVI; i++) {
                kill(abs(shm_pid_array[i]), SIGINT);
            }
            kill(pid_weather, SIGINT);
            exit(EXIT_FAILURE);
        case SIGALRM:
            printf("Day [%d]/[%d].\n", ++shm_cfg->CURRENT_DAY, shm_cfg->SO_DAYS);

            /* Simulation has reached its end */
            if(shm_cfg->CURRENT_DAY == shm_cfg->SO_DAYS) {
                for(i = 0; i < shm_cfg->SO_PORTI + shm_cfg->SO_NAVI; i++) {
                    kill(abs(shm_pid_array[i]), SIGTERM);
                }
                kill(pid_weather, SIGTERM);
                return;
            }

            /* Counts how many requests and offers there are */
            for (i = 0, check_port_offers = 0, check_port_request = 0; i < shm_cfg->SO_PORTI * shm_cfg->SO_MERCI && !(check_port_offers && check_port_request); i++) {
                if(shm_goods[i] < 0) check_port_request += -shm_goods[i];
                else if (shm_goods[i] > 0) check_port_offers += shm_goods[i];
            }

            fprintf(output, "%d\t%d\n", check_port_offers, check_port_request);

            if (!check_port_request || !check_port_offers) {
                for(i = 0; i < shm_cfg->SO_PORTI + shm_cfg->SO_NAVI; i++) {
                    kill(abs(shm_pid_array[i]), SIGTERM);
                }
                kill(pid_weather, SIGTERM);
                if(!check_port_offers) printf("No more offers are available \n");
                else printf("No more requests are available \n");
                return;
            }


            kill(pid_weather, SIGALRM);
            generate_goods(); /* Generate goods for the next day */

            CHECK_ERROR_MASTER(semctl(shm_cfg->sem_id_gen_precedence, 0, SETVAL,
                                      shm_cfg->SO_PORTI + shm_cfg->SO_NAVI - shm_dump_ships->sunk) < 0,
                        "[MASTER] Error while setting the semaphore for dump control in SIGALRM")

            /* Tell all processes that a day has passed (is about to pass) */
            for(i = 0; i < shm_cfg->SO_PORTI + shm_cfg->SO_NAVI; i++) {
                kill(abs(shm_pid_array[i]), SIGALRM);
            }

            while (sem_cmd(shm_cfg->sem_id_gen_precedence, 0, 0, 0)) {
                CHECK_ERROR_MASTER(errno != EINTR, "[MASTER] Error while waiting the semaphore for dump control in SIGALRM")
            }

            print_dump();
            shm_dump_ships->with_cargo_en_route = 0;
            shm_dump_ships->without_cargo_en_route = 0;
            shm_dump_ships->being_loaded_unloaded = 0;
            alarm(shm_cfg->SO_DAY_LENGTH);

            /* Weather is automatically frozen after it completes its task inside SIGALRM */
            kill(pid_weather, SIGCONT);
            break;
        case SIGUSR1:
            printf("All ships have sunk\n");
            printf("Day [%d]/[%d].\n", shm_cfg->CURRENT_DAY, shm_cfg->SO_DAYS);
            for(i = 0; i < shm_cfg->SO_PORTI; i++) {
                kill(abs(shm_pid_array[i]), SIGTERM);
            }
            print_dump();
            break;
        default:
            printf("[MASTER] Signal: %s\n", strsignal(signum));
            break;
    }
    errno = old_errno;
}
