#include "../headers/master.h"
#include "../headers/utils.h"
#include "../headers/common_ipcs.h"

void meteo_sig_handler(int);

config  *shm_cfg;
pid_t   *shm_pid_array;

int shm_id_config;
int shm_id_pid_array;
int sem_id_pid_mutex;
int available_ships;

int main(int argc, char** argv) {
    int index_pid_to_term;
    pid_t pid_ship_to_term;

    struct sigaction sa;
    struct timespec maelstorm_duration, rem;

    if(argc != 3) {
        printf("Incorrect number of parameters [%d]. Exiting...\n", argc);
        kill(getppid(), SIGINT);
    }

    /* TODO: CONTROLLARE IL FALLIMENTO DI QUESTA SEZIONE DI CODICE UNA VOLTA FATTO IL REFACTORING */
    shm_id_config = string_to_int(argv[1]);
    shm_id_pid_array = string_to_int(argv[2]);
    /*sem_id_pid_mutex = string_to_int(argv[3]); */

    CHECK_ERROR((shm_cfg = shmat(shm_id_config, NULL, 0)) == (void*) -1, getppid(),
                "[METEO] Error while trying to attach to configuration shared memory")

    CHECK_ERROR((shm_pid_array = shmat(shm_id_pid_array, NULL, 0)) == (void*) -1, getppid(),
                "[METEO] Error while trying to attach to configuration shared memory")

    available_ships = shm_cfg->SO_NAVI;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = meteo_sig_handler;
    sa.sa_flags = SA_RESTART;

    sigaction(SIGALRM, &sa, NULL);

    pause();

    while (available_ships) {
        maelstorm_duration = calculate_timeout(shm_cfg->SO_MAELSTROM, shm_cfg->SO_DAY_LENGTH);
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
        index_pid_to_term = ((int) random() % available_ships) + shm_cfg->SO_PORTI;
        printf("[METEO] index to kill: %d\n", (index_pid_to_term - shm_cfg->SO_PORTI));
        /*while (sem_cmd(sem_id_pid_mutex, index_pid_to_term, -1, 0)); */
        pid_ship_to_term = shm_pid_array[index_pid_to_term];
        shm_pid_array[index_pid_to_term] = shm_pid_array[available_ships + shm_cfg->SO_PORTI - 1];
        shm_pid_array[available_ships + shm_cfg->SO_PORTI - 1] = -1;
        available_ships--;
        kill(pid_ship_to_term, SIGTERM);
    }
    kill(getppid(), SIGUSR1);
    return 0;
}

void meteo_sig_handler(int signum) {
    int old_errno = errno;

    switch (signum) {
        case SIGALRM:
            kill(shm_pid_array[random() % shm_cfg->SO_PORTI], SIGUSR1);
            break;
        default:
            break;
    }

    errno = old_errno;
}
