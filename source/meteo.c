#include "../headers/master.h"
#include "../headers/utils.h"
#include "../headers/common_ipcs.h"

void meteo_sig_handler(int);

config  *shm_cfg;
pid_t   *shm_pid_array;
dump_ships *shm_dump_ships;

int shm_id_config;

int main(int argc, char** argv) {
    int i;
    int index_pid_to_term;
    int available_ships;
    int *index_pid_status;

    struct sigaction sa;
    struct timespec maelstorm_duration, rem;

    if(argc != 2) {
        printf("Incorrect number of parameters [%d]. Exiting...\n", argc);
        kill(getppid(), SIGINT);
    }

    shm_id_config = string_to_int(argv[1]);
    CHECK_ERROR_CHILD(errno, "[METEO] Error while trying to convert shm_id_config")

    CHECK_ERROR_CHILD((shm_cfg = shmat(shm_id_config, NULL, 0)) == (void*) -1,
                      "[METEO] Error while trying to attach to configuration shared memory")

    CHECK_ERROR_CHILD((shm_pid_array = shmat(shm_cfg->shm_id_pid_array, NULL, 0)) == (void*) -1,
                      "[METEO] Error while trying to attach to configuration shared memory")

    CHECK_ERROR_CHILD((shm_dump_ships = shmat(shm_cfg->shm_id_dump_ships, NULL, 0)) == (void*) -1,
                      "[METEO] Error while trying to attach to ships dump shared memory")

    available_ships = shm_cfg->SO_NAVI;
    index_pid_status = malloc(sizeof(shm_cfg->SO_NAVI) * shm_cfg->SO_NAVI);

    for(i = 0; i < shm_cfg->SO_NAVI; i++) {
        index_pid_status[i] = i;
    }

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = meteo_sig_handler;
    sa.sa_flags = SA_RESTART;

    sigaction(SIGALRM, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGCONT, &sa, NULL);

    pause();

    while (available_ships) {
        maelstorm_duration = calculate_timeout(shm_cfg->SO_MAELSTORM, shm_cfg->SO_DAY_LENGTH);
        printf("[METEO] Maelstorm duration: %lds:%ldns\n", maelstorm_duration.tv_sec, maelstorm_duration.tv_nsec);
        while (nanosleep(&maelstorm_duration, &rem)) {
            switch (errno) {
                case EINTR:
                    maelstorm_duration = rem;
                    continue;
                default:
                    perror("[METEO] Generic error in nanosleep\n");
                    kill(getppid(), SIGINT);
                    break;
            }
        }
        index_pid_to_term = (int) random() % available_ships;
        printf("[METEO] index to kill: %d\n", index_pid_to_term);
        /*while (sem_cmd(sem_id_pid_mutex, index_pid_status[index_pid_to_term], -1, 0)); */
        kill(shm_pid_array[index_pid_status[index_pid_to_term] + shm_cfg->SO_PORTI], SIGTERM);
        /*while (sem_cmd(sem_id_pid_mutex, index_pid_status[index_pid_to_term], 1, 0)); */
        index_pid_status[index_pid_to_term] = index_pid_status[--available_ships];
        shm_dump_ships->ships_sunk++;
    }
    kill(getppid(), SIGUSR1);
    return 0;
}

void meteo_sig_handler(int signum) {
    int old_errno = errno;

    switch (signum) {
        case SIGCONT:
            break;
        case SIGALRM:
            kill(shm_pid_array[random() % shm_cfg->SO_PORTI], SIGUSR1);
            /*TODO: in questo momento tutte le navi possono essere fermate, non solo quelle che navigano*/
            kill(shm_pid_array[random() % shm_cfg->SO_NAVI + shm_cfg->SO_PORTI], SIGUSR1);
            shm_dump_ships->ships_slowed++;
            CHECK_ERROR_CHILD(sem_cmd(shm_cfg->sem_id_gen_precedence, 0, -1, 0) < 0,
                              "[METEO] Error while trying to release sem_id_gen_precedence")
            break;
        case SIGTERM:
            exit(EXIT_SUCCESS);
        default:
            break;
    }

    errno = old_errno;
}
