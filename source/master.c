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
void initialize_goods_template(void);
void initialize_ports_coords(void);
void selected_prints(void);
void final_print(void);

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
    int wstatus;
    pid_t p_wait;
    struct sigaction sa;

    if(argc != 2) {
        printf("[MASTER] Incorrect number of parameters [%d]. Re-execute with: {configuration file}\n", argc);
        exit(EXIT_FAILURE);
    }

    /* Initialize sigaction struct */
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = master_sig_handler;
    sa.sa_flags = SA_RESTART;
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGALRM, &sa, NULL);
    sigaction(SIGUSR1, &sa, NULL);

    srandom(getpid());

    CHECK_ERROR_MASTER(atexit(clear_all), "[MASTER] Error while trying to register clear_all function to atexit")

    initialize_so_vars(argv[1]);
    initialize_goods_template();

    CHECK_ERROR_MASTER(!(output = fopen("output.txt", "w")),
                       "[MASTER] Error while trying to create/rewrite output.txt")

    /* create and attach pid shm for the children */
    CHECK_ERROR_MASTER((shm_cfg->shm_id_pid_array =
                                shmget(IPC_PRIVATE, sizeof(pid_t) * (shm_cfg->SO_PORTI + shm_cfg->SO_NAVI), 0600)) < 0,
                       "[MASTER] Error while creating pid array shared memory")
    CHECK_ERROR_MASTER((shm_pid_array = shmat(shm_cfg->shm_id_pid_array, NULL, 0)) == (void *) -1,
                       "[MASTER] Error while trying to attach to pid array shared memory")


    /* create and attach pid shm for the goods tracking offers */
    CHECK_ERROR_MASTER((shm_cfg->shm_id_goods =
                                shmget(IPC_PRIVATE, sizeof(int) * shm_cfg->SO_PORTI * shm_cfg->SO_MERCI, 0600)) < 0,
                       "[MASTER] Error while creating goods_tracking shared memory")
    CHECK_ERROR_MASTER((shm_goods = shmat(shm_cfg->shm_id_goods, NULL, 0)) == (void *) -1,
                       "[MASTER] Error while trying to attach to goods_tracking shared memory")
    memset(shm_goods, 0, sizeof(int) * shm_cfg->SO_PORTI * shm_cfg->SO_MERCI);

    /* create and attach pid shm for dump report */
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
    /* deletes all the contents of shared memory arrays */
    memset(shm_dump_ports, 0, sizeof(dump_ports) * shm_cfg->SO_PORTI);
    memset(shm_dump_ships, 0, sizeof(dump_ships));
    memset(shm_dump_goods, 0, sizeof(dump_goods) * shm_cfg->SO_MERCI);

    /* creates the messages queues for the handshake between ports and ships of the ports */
    CHECK_ERROR_MASTER((shm_cfg->mq_id_ports_handshake = msgget(IPC_PRIVATE, 0600)) < 0,
                       "[MASTER] Error while creating ports_handshake message queue")
    CHECK_ERROR_MASTER((shm_cfg->mq_id_ships_handshake = msgget(IPC_PRIVATE, 0600)) < 0,
                       "[MASTER] Error while creating ships_handshake message queue")
    CHECK_ERROR_MASTER((shm_cfg->mq_id_ships_goods = msgget(IPC_PRIVATE, 0600)) < 0,
                       "[MASTER] Error while creating ships_goods message queue")

    /* create the semaphore array to control access to current requests */
    CHECK_ERROR_MASTER((shm_cfg->sem_id_check_request = semget(IPC_PRIVATE, shm_cfg->SO_PORTI, 0600)) < 0,
                       "[MASTER] Error while creating semaphore array for requests")
    /* create the semaphore for process generation control and the dump status */
    CHECK_ERROR_MASTER((shm_cfg->sem_id_gen_precedence = semget(IPC_PRIVATE, 1, 0600)) < 0,
                       "[MASTER] Error while creating semaphore for generation order control")
    /* create the semaphore for dock generation control and the dump status */
    CHECK_ERROR_MASTER((shm_cfg->sem_id_dock = semget(IPC_PRIVATE, shm_cfg->SO_PORTI, 0600)) < 0,
                       "[MASTER] Error while creating semaphore for docks control")

    set_mutex_sem_array(shm_cfg->sem_id_check_request, shm_cfg->SO_PORTI);

    /* Initialize generation semaphore at the value of SO_PORTI */
    CHECK_ERROR_MASTER(semctl(shm_cfg->sem_id_gen_precedence, 0, SETVAL, shm_cfg->SO_PORTI),
                       "[MASTER] Error while setting the semaphore for ports generation control")

    create_ports();
    printf("Waiting for ports generation...\n");

    CHECK_ERROR_MASTER(sem_cmd(shm_cfg->sem_id_gen_precedence, 0, 0, 0),
                       "[MASTER] Error while waiting for ports generation")

    CHECK_ERROR_MASTER(semctl(shm_cfg->sem_id_gen_precedence, 0, SETVAL, shm_cfg->SO_NAVI),
                       "[MASTER] Error while setting the semaphore for ships generation control")

    create_ships();
    printf("Waiting for ships generation...\n");

    CHECK_ERROR_MASTER(sem_cmd(shm_cfg->sem_id_gen_precedence, 0, 0, 0),
                       "[MASTER] Error while waiting for ships generation")

    create_weather();

    /* generates day 0 goods */
    generate_goods();

    printf("INIZIO SIMULAZIONE!\n");
    CHECK_ERROR_MASTER(killpg(0, SIGCONT) && (errno != ESRCH), "[MASTER] Error while trying killpg")

    /* day 1 alarm */
    alarm(shm_cfg->SO_DAY_LENGTH);

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

    final_print();
    printf("FINE SIMULAZIONE!\n\n");
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
    CHECK_ERROR_MASTER(semctl(shm_cfg->sem_id_check_request, 0, IPC_RMID, 0),
                       "[MASTER] Error while removing semaphore for check request control in clear_all")
}

void initialize_so_vars(char* path_cfg_file) {
    FILE *fp;
    char buffer[BUFFER_SIZE];
    int check = 0;

    /* Check if the file path in argv1 exists */
    if (access(path_cfg_file, F_OK | R_OK)) {
        perror("[MASTER] Error while trying to access the configuration file");
        exit(EXIT_FAILURE);
    }

    CHECK_ERROR_MASTER(!(fp = fopen(path_cfg_file, "r")), "Error during: initialize_so_vars->fopen()")

    /* create and attach config shared memory segment */
    if ((shm_id_config = shmget(IPC_PRIVATE, sizeof(*shm_cfg), 0600)) < 0) {
        perror("[MASTER] Error while creating shared memory for configuration parameters");
        exit(EXIT_FAILURE);
    }
    CHECK_ERROR_MASTER((shm_cfg = shmat(shm_id_config, NULL, 0)) == (void *) -1,
                       "[MASTER] Error while trying to attach to configuration shared memory")

    while (!feof(fp)) {
        if(fgets(buffer, BUFFER_SIZE, fp) == NULL) {
            break;
        }

        if(sscanf(buffer, "#%s", buffer) == 1) {
            continue;
        }

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
        } else if (sscanf(buffer, "SO_PRINT_PORTS: %d", &shm_cfg->SO_PRINT_PORTS) == 1) {
            check |= 1 << 17;
        } else if (sscanf(buffer, "SO_PRINT_GOODS: %d", &shm_cfg->SO_PRINT_GOODS) == 1) {
            check |= 1 << 18;
        }
    }
    CHECK_ERROR_MASTER(fclose(fp), "[MASTER] Error while closing the fp")

    errno = EINVAL;
    CHECK_ERROR_MASTER(check != 0x7FFFF, "[MASTER] Missing config")
    CHECK_ERROR_MASTER(shm_cfg->SO_NAVI < 1, "[MASTER] SO_NAVI is less than 1")
    CHECK_ERROR_MASTER(shm_cfg->SO_PORTI < 4, "[MASTER] SO_PORTI is less than 4")
    CHECK_ERROR_MASTER(shm_cfg->SO_PORTI + shm_cfg->SO_NAVI + 1 > sysconf(_SC_CHILD_MAX),
                       "[MASTER] Too many processes than the _SC_CHILD_MAX limit")
    CHECK_ERROR_MASTER(shm_cfg->SO_MERCI <= 0, "[MASTER] SO_MERCI is less or equal than 0")
    CHECK_ERROR_MASTER(shm_cfg->SO_MIN_VITA <= 0, "[MASTER] SO_MIN_VITA is less or equal than 0")
    CHECK_ERROR_MASTER(shm_cfg->SO_MIN_VITA > shm_cfg->SO_MAX_VITA, "[MASTER] SO_MIN_VITA is greater than SO_MAX_VITA")
    CHECK_ERROR_MASTER(shm_cfg->SO_LATO <= 0, "[MASTER] SO_LATO is less or equal than 0")
    CHECK_ERROR_MASTER(shm_cfg->SO_SPEED <= 0, "[MASTER] SO_SPEED is less or equal than 0")
    CHECK_ERROR_MASTER(shm_cfg->SO_CAPACITY <= 0, "[MASTER] SO_CAPACITY is less or equal than 0")
    CHECK_ERROR_MASTER(shm_cfg->SO_BANCHINE <= 0, "[MASTER] SO_BANCHINE is less or equal than 0")
    CHECK_ERROR_MASTER(shm_cfg->SO_FILL <= 0, "[MASTER] SO_FILL is less or equal than 0")
    CHECK_ERROR_MASTER(shm_cfg->SO_LOADSPEED <= 0, "[MASTER] SO_LOADSPEED is less or equal than 0")
    CHECK_ERROR_MASTER(shm_cfg->SO_DAYS <= 0, "[MASTER] SO_DAYS is less or equal than 0")
    CHECK_ERROR_MASTER(shm_cfg->SO_DAY_LENGTH <= 0, "[MASTER] SO_DAY_LENGTH is less or equal than 0")
    CHECK_ERROR_MASTER(shm_cfg->SO_STORM_DURATION < 0, "[MASTER] SO_STORM_DURATION is less than 0")
    CHECK_ERROR_MASTER(shm_cfg->SO_SWELL_DURATION < 0, "[MASTER] SO_SWELL_DURATION is less than 0")
    CHECK_ERROR_MASTER(shm_cfg->SO_MAELSTORM < 0, "[MASTER] SO_MAELSTORM is less than 0")
    CHECK_ERROR_MASTER(shm_cfg->SO_PRINT_PORTS < 0, "[MASTER] SO_PRINT_PORTS is less than 0")
    CHECK_ERROR_MASTER(shm_cfg->SO_PRINT_GOODS < 0, "[MASTER] SO_PRINT_GOODS is less than 0")
    CHECK_ERROR_MASTER(shm_cfg->SO_PRINT_PORTS > shm_cfg->SO_PORTI, "[MASTER] SO_PRINT_PORTS is more than SO_PORTI")
    CHECK_ERROR_MASTER(shm_cfg->SO_PRINT_GOODS > shm_cfg->SO_MERCI, "[MASTER] SO_PRINT_GOODS is more than SO_merci")

    shm_cfg->CURRENT_DAY = 0;
    errno = 0;
}

/* create and attach goods_template shared memory segment */
void initialize_goods_template(void) {
    int i;

    CHECK_ERROR_MASTER((shm_cfg->shm_id_goods_template = shmget(IPC_PRIVATE,
                                                                sizeof(goods) * shm_cfg->SO_MERCI, 0600)) < 0,
                       "[MASTER] Error while creating shared memory for goods_template generation")
    CHECK_ERROR_MASTER((shm_goods_template = shmat(shm_cfg->shm_id_goods_template, NULL, 0)) == (void *) -1,
                       "[MASTER] Error while trying to attach to goods template shared memory")

    /* initialize goods_template array */
    for (i = 0; i < shm_cfg->SO_MERCI; i++) {
        shm_goods_template[i].tons = (int) random() % shm_cfg->SO_SIZE + 1;
        shm_goods_template[i].lifespan = (int) random() % (shm_cfg->SO_MAX_VITA - shm_cfg->SO_MIN_VITA + 1)
                                         + shm_cfg->SO_MIN_VITA;
    }
    qsort(shm_goods_template, shm_cfg->SO_MERCI, sizeof(goods_template), compare_goods_template);

}

void initialize_ports_coords(void) {
    int i, j;

    /* create and attach ports coordinate shared memory segment */
    CHECK_ERROR_MASTER((shm_cfg->shm_id_ports_coords = shmget(IPC_PRIVATE,
                                                              sizeof(*shm_ports_coords) * shm_cfg->SO_PORTI, 0600)) < 0,
                       "[MASTER] Error while creating shared memory for ports coordinates")
    CHECK_ERROR_MASTER((shm_ports_coords = shmat(shm_cfg->shm_id_ports_coords, NULL, 0)) == (void *) -1,
                       "[MASTER] Error while trying to attach to ports coordinates shared memory")


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
        shm_ports_coords[i].x = (double) random() / RAND_MAX * shm_cfg->SO_LATO;
        shm_ports_coords[i].y = (double) random() / RAND_MAX * shm_cfg->SO_LATO;

        for(j = 0; j < i && !(shm_ports_coords[j].x == shm_ports_coords[i].x && shm_ports_coords[j].y == shm_ports_coords[i].y); j++);

        if(j < i) i--;
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

    initialize_ports_coords();

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
    /* Clear up the memory allocated for the arguments */
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

    /* Clear up the memory allocated for the arguments */
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

    /* Clear up the memory allocated for the arguments */
    free(args[1]);
}

void print_dump(void) {
    int i, sum = 0;
    fprintf(output, "###### DUMP:\n");
    fprintf(output, "---------------------------------MERCI---------------------------------\n");
    for(i = 0; i < shm_cfg->SO_MERCI; i++) {
        fprintf(output,"ID: [%d]\tSTATE: [good_delivered: %d  |  good_in_port: %d  |  good_on_ship: %d  |  good_expired_in_port: %d  |  good_expired_on_ship: %d]\n", i, shm_dump_goods[i].good_delivered,
                shm_dump_goods[i].good_in_port, shm_dump_goods[i].good_on_ship, shm_dump_goods[i].good_expired_in_port, shm_dump_goods[i].good_expired_on_ship);
        fprintf(output,"---------------------------\n");
    }
    fprintf(output,"---------------------------------PORTI---------------------------------\n");
    for(i = 0; i < shm_cfg->SO_PORTI; i++) {
        fprintf(output,"ID: [%d]\tON_SWELL: [%d]\n", i, shm_dump_ports[i].on_swell);
        fprintf(output,"DOCK: [%d/%d]\n", shm_dump_ports[i].dock_available, shm_dump_ports[i].dock_total);
        fprintf(output,"GOODS: [goods_available: %d  |  good_send: %d  |  good_received: %d  |  ton_in_excess_offers: %d |  ton_in_excess_request: %d]\n",
                shm_dump_ports[i].good_available, shm_dump_ports[i].good_send,
                shm_dump_ports[i].good_received, shm_dump_ports[i].ton_in_excess_offers, shm_dump_ports[i].ton_in_excess_request);
        fprintf(output,"-----------\n");
    }
    fprintf(output,"---------------------------------NAVI---------------------------------\n");
    fprintf(output,"SHIPS: [Ship with cargo: %d  |  Ship without cargo: %d  |  Ship loading or unloading: %d]\n", shm_dump_ships->with_cargo_en_route,
            shm_dump_ships->without_cargo_en_route, shm_dump_ships->being_loaded_unloaded);
    fprintf(output,"SHIPS: [SLOWED: %d  |  SUNK: %d]\n", shm_dump_ships->slowed, shm_dump_ships->sunk);
    fprintf(output,"----------------------------------------------------------------------\n");

    for (i = 0; i < shm_cfg->SO_PORTI; i++) {
        sum += shm_dump_ports[i].ton_in_excess_offers + shm_dump_ports[i].total_goods_offers;
    }

    printf("Total goods: %d\n", sum);
    selected_prints();
}

void selected_prints(void) {
    int *arraySelected = calloc(shm_cfg->SO_PRINT_PORTS, sizeof(int));
    int *arraySelectedGoods = calloc(shm_cfg->SO_PRINT_GOODS, sizeof(int));
    int i, j, rng;
    printf("-----------------------------------------------------------------Stampa di %d PORTI: \n", shm_cfg->SO_PRINT_PORTS);

    for (i = 0; i < shm_cfg->SO_PRINT_PORTS ; i++) {
        rng = (int) random() % shm_cfg->SO_PORTI;
        for (j = 0; j < i; j++) {
            if(arraySelected[j] == rng) {
                break;
            }
        }
        if (i == j) arraySelected[i] = rng;
        else i--;
    }

    for(i = 0; i < shm_cfg->SO_PRINT_PORTS; i++) {
        printf("ID: [%d]\tON_SWELL: [%d]\n", arraySelected[i], shm_dump_ports[arraySelected[i]].on_swell);
        printf("DOCK: [%d/%d]\n", shm_dump_ports[arraySelected[i]].dock_available, shm_dump_ports[arraySelected[i]].dock_total);
        printf("GOODS: [goods_available: %d  |  good_send: %d  |  good_received: %d  |  ton_in_excess_offers: %d |  ton_in_excess_request: %d]\n",
               shm_dump_ports[arraySelected[i]].good_available, shm_dump_ports[arraySelected[i]].good_send,
               shm_dump_ports[arraySelected[i]].good_received, shm_dump_ports[arraySelected[i]].ton_in_excess_offers, shm_dump_ports[arraySelected[i]].ton_in_excess_request);
        printf("----------------------------------------------\n");
    }
    free(arraySelected);

    for (i = 0; i < shm_cfg->SO_PRINT_GOODS ; i++) {
        rng = (int) random() % shm_cfg->SO_MERCI;
        for (j = 0; j < i; j++) {
            if(arraySelectedGoods[j] == rng) {
                break;
            }
        }
        if (i == j) arraySelectedGoods[i] = rng;
        else i--;
    }

    printf("-----------------------------------------------------------------Stampa di %d MERCI: \n", shm_cfg->SO_PRINT_GOODS);
    for (i = 0; i < shm_cfg->SO_PRINT_GOODS ; i++) {
        printf("ID: [%d]\tSTATE: [good_delivered: %d  |  good_in_port: %d  |  good_on_ship: %d  |  good_expired_in_port: %d  |  good_expired_on_ship: %d]\n", arraySelectedGoods[i], shm_dump_goods[arraySelectedGoods[i]].good_delivered,
               shm_dump_goods[arraySelectedGoods[i]].good_in_port, shm_dump_goods[arraySelectedGoods[i]].good_on_ship, shm_dump_goods[arraySelectedGoods[i]].good_expired_in_port, shm_dump_goods[arraySelectedGoods[i]].good_expired_on_ship);
        printf("----------------------------------------------\n");
    }
    free(arraySelectedGoods);
}

void final_print(void) {
    int sum = 0;
    int i;
    int bestOfferer;
    int bestReceiver;

    double delivered_goods = 0;
    double lost_goods = 0;
    double total_goods = 0;

    printf("-------------FINAL DUMPS-------------\n");
    printf("Ships at sea at the end of the simulation: %d\n", (shm_dump_ships->with_cargo_en_route + shm_dump_ships->without_cargo_en_route));
    printf("Number of ships still at sea with a cargo on board: %d\n", shm_dump_ships->with_cargo_en_route);
    printf("Number of ships still at sea without a cargo: %d\n", shm_dump_ships->without_cargo_en_route);
    printf("Number of ships occupying a dock: %d\n", shm_dump_ships->being_loaded_unloaded);
    printf("Number of ships sunk: %d\n", shm_dump_ships->sunk);
    printf("---Final status of goods:\n");
    for (i = 0; i < shm_cfg->SO_PORTI; i++) {
        sum += shm_dump_ports[i].ton_in_excess_offers + shm_dump_ports[i].total_goods_offers;
        total_goods = shm_dump_ports[i].total_goods_offers;
    }
    printf("Total goods: %d\n", sum);
    fprintf(output,"-------------FINAL DUMPS-------------\n");
    fprintf(output,"Ships at sea at the end of the simulation: %d\n", (shm_dump_ships->with_cargo_en_route + shm_dump_ships->without_cargo_en_route));
    fprintf(output,"Number of ships still at sea with a cargo on board: %d\n", shm_dump_ships->with_cargo_en_route);
    fprintf(output,"Number of ships still at sea without a cargo: %d\n", shm_dump_ships->without_cargo_en_route);
    fprintf(output,"Number of ships occupying a dock: %d\n", shm_dump_ships->being_loaded_unloaded);
    fprintf(output,"Number of ships sunk: %d\n", shm_dump_ships->sunk);
    fprintf(output,"---Final status of goods:\n");
    for(i = 0; i < shm_cfg->SO_MERCI; i++) {
        printf("ID: [%d]\tSTATE: [good_delivered: %d  |  good_in_port: %d  |  good_on_ship: %d  |  good_expired_in_port: %d  |  good_expired_on_ship: %d]\n", i, shm_dump_goods[i].good_delivered,
               shm_dump_goods[i].good_in_port, shm_dump_goods[i].good_on_ship, shm_dump_goods[i].good_expired_in_port, shm_dump_goods[i].good_expired_on_ship);
        fprintf(output,"ID: [%d]\tSTATE: [good_delivered: %d  |  good_in_port: %d  |  good_on_ship: %d  |  good_expired_in_port: %d  |  good_expired_on_ship: %d]\n", i, shm_dump_goods[i].good_delivered,
                shm_dump_goods[i].good_in_port, shm_dump_goods[i].good_on_ship, shm_dump_goods[i].good_expired_in_port, shm_dump_goods[i].good_expired_on_ship);
        delivered_goods += shm_dump_goods[i].good_delivered;
        lost_goods += shm_dump_goods[i].good_expired_in_port + shm_dump_goods[i].good_expired_on_ship;
    }
    printf("---The best port for generated supply and generated demand: \n");
    fprintf(output, "---The best port for generated supply and generated demand: \n");
    /*un for che scorre per ogni porto la best quantità totale di merce generata e la quantita totale di merci generate*/
    for(i = 1, bestOfferer = bestReceiver = 0; i < shm_cfg->SO_PORTI; i++){
        if(shm_dump_ports[i].total_goods_offers > shm_dump_ports[bestOfferer].total_goods_offers) {
            bestOfferer = i;
        } else if(shm_dump_ports[i].total_goods_requested > shm_dump_ports[bestReceiver].total_goods_requested) {
            bestReceiver = i;
        }
    }
    /*lost goods, delivered goods sunk/expected */
    printf("Actual sunk / expected sunk: \t[%f%%]\n", (100 * shm_dump_ships->sunk / (24.0 / shm_cfg->SO_MAELSTORM * shm_cfg->SO_DAYS)));
    printf("Delivered goods: \t[%f%%]\n", (delivered_goods / total_goods));
    printf("Lost goods: \t[%f%%]\n", (lost_goods / total_goods));
    printf("Best port for the generated offer id: %d  --> %d\n", bestOfferer, shm_dump_ports[bestOfferer].total_goods_offers);
    printf("Best port for the generated request id: %d --> %d\n", bestReceiver, shm_dump_ports[bestReceiver].total_goods_requested);
    fprintf(output,"Best port for the generated offer id: %d  --> %d\n", bestOfferer, shm_dump_ports[bestOfferer].total_goods_offers);
    fprintf(output,"Best port for the generated request id: %d --> %d\n", bestReceiver, shm_dump_ports[bestReceiver].total_goods_requested);
}

void generate_goods(void) {
    int i;
    shm_cfg->GENERATING_PORTS = (int) random() % shm_cfg->SO_PORTI;

    for (i = 0; i < shm_cfg->GENERATING_PORTS; i++) {
        int index;

        while ((index = (int) random() % shm_cfg->SO_PORTI) && shm_pid_array[index] < 0);

        shm_pid_array[index] = -shm_pid_array[index];
    }

    printf("[MASTER] GENERATING_PORTS: [%d]\n", shm_cfg->GENERATING_PORTS);
}

void master_sig_handler(int signum) {
    int old_errno = errno;
    int i, check_port_offers, check_port_request;

    switch(signum) {
        case SIGTERM:
        case SIGINT:
            printf("Error has occurred... killing all processes.\n");
            for(i = 0; i < shm_cfg->SO_PORTI + shm_cfg->SO_NAVI; i++) {
                CHECK_ERROR_MASTER(kill(abs(shm_pid_array[i]), SIGINT) && (errno != ESRCH), "[MASTER] Error fail kill in the shm_pid_array")
            }
            CHECK_ERROR_MASTER(kill(pid_weather, SIGINT) && (errno != ESRCH), "[MASTER] Error fail kill meteo")
            exit(EXIT_FAILURE);
            /* Still needs to deal with statistics first */
        case SIGALRM:
            printf("\x1b[32m Day [%d]/[%d].\x1b[0m\n", ++shm_cfg->CURRENT_DAY, shm_cfg->SO_DAYS);

            if(shm_cfg->CURRENT_DAY == shm_cfg->SO_DAYS) {
                for(i = 0; i < shm_cfg->SO_PORTI + shm_cfg->SO_NAVI; i++) {
                    CHECK_ERROR_MASTER(kill(abs(shm_pid_array[i]), SIGTERM) && (errno != ESRCH), "[MASTER] Error while sending sigterm to children")
                }
                CHECK_ERROR_MASTER(kill(pid_weather, SIGTERM) && (errno != ESRCH), "[MASTER] Error while sending sigterm to meteo")
                return;
            }

            for (i = 0, check_port_offers = 0, check_port_request = 0; i < shm_cfg->SO_PORTI * shm_cfg->SO_MERCI && !(check_port_offers && check_port_request); i++) {
                if(shm_goods[i] < 0) check_port_request += -shm_goods[i];
                else if (shm_goods[i] > 0) check_port_offers += shm_goods[i];
            }

            fprintf(output, "Total port offers: %d\tTotal port requests: %d\n", check_port_offers, check_port_request);

            if (!check_port_request || !check_port_offers) {
                for(i = 0; i < shm_cfg->SO_PORTI + shm_cfg->SO_NAVI; i++) {
                    CHECK_ERROR_MASTER(kill(abs(shm_pid_array[i]), SIGTERM) && (errno != ESRCH), "[MASTER] Error while sending sigterm to children")
                }
                CHECK_ERROR_MASTER(kill(pid_weather, SIGTERM) && (errno != ESRCH), "[MASTER] Error while sending sigterm to meteo")
                if(!check_port_offers) printf("NEI PORTI NON SONO PRESENTI PIU' OFFERTE\n");
                else printf("NEI PORTI NON SONO PRESENTI PIU' RICHIESTE\n");
                return;
            }

            CHECK_ERROR_MASTER(kill(pid_weather, SIGALRM) && (errno != ESRCH), "[MASTER] Error while sending sigalarm to meteo")
            print_dump();
            generate_goods();

            CHECK_ERROR_MASTER(semctl(shm_cfg->sem_id_gen_precedence, 0, SETVAL,
                                      shm_cfg->SO_PORTI + shm_cfg->SO_NAVI - shm_dump_ships->sunk) < 0,
                               "[MASTER] Error while setting the semaphore for dump control in SIGALRM")

            for(i = 0; i < shm_cfg->SO_PORTI + shm_cfg->SO_NAVI; i++) {
                CHECK_ERROR_MASTER(kill(abs(shm_pid_array[i]), SIGALRM) && (errno != ESRCH), "[MASTER] Error while sending sigalarm to children")
            }

            while (sem_cmd(shm_cfg->sem_id_gen_precedence, 0, 0, 0)) {
                CHECK_ERROR_MASTER(errno != EINTR, "[MASTER] Error while waiting the semaphore for dump control in SIGALRM")
            }

            /* Check SO_DAYS against the current day. If they're the same kill everything */
            shm_dump_ships->with_cargo_en_route = 0;
            shm_dump_ships->without_cargo_en_route = 0;
            shm_dump_ships->being_loaded_unloaded = 0;
            alarm(shm_cfg->SO_DAY_LENGTH);

            CHECK_ERROR_MASTER(kill(pid_weather, SIGCONT) && (errno != ESRCH), "[MASTER] Error while sending sigcont to meteo")
            break;
        case SIGUSR1:
            printf("ALL SHIPS ARE DEAD :C\n");
            printf("Day [%d]/[%d].\n", shm_cfg->CURRENT_DAY, shm_cfg->SO_DAYS);
            for(i = 0; i < shm_cfg->SO_PORTI; i++) {
                CHECK_ERROR_MASTER(kill(abs(shm_pid_array[i]), SIGTERM) && (errno != ESRCH), "[MASTER] Error while sending sigterm to children")
            }
            return;
        default:
            printf("[MASTER] Signal: %s\n", strsignal(signum));
            break;
    }
    errno = old_errno;
}
