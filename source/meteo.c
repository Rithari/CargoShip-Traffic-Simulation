#include "../headers/master.h"
#include "../headers/utils.h"
#include "../headers/common_ipcs.h"

void meteo_sig_handler(int);

config  *shm_cfg;
pid_t   *shm_pid_array;
dump_ships *shm_dump_ships;

int shm_id_config;
int *index_pid_status;
unsigned int available_ships;

int main(int argc, char** argv) {
    int i;
    unsigned int index_pid_to_term;

    struct sigaction sa;

    if(argc != 2) {
        printf("Incorrect number of parameters [%d]. Exiting...\n", argc);
        kill(getppid(), SIGINT);
    }

    srandom(getpid());

    shm_id_config = string_to_int(argv[1]);
    CHECK_ERROR_CHILD(errno, "[METEO] Error while trying to convert shm_id_config")

    CHECK_ERROR_CHILD((shm_cfg = shmat(shm_id_config, NULL, 0)) == (void*) -1,
                      "[METEO] Error while trying to attach to configuration shared memory")

    CHECK_ERROR_CHILD((shm_pid_array = shmat(shm_cfg->shm_id_pid_array, NULL, 0)) == (void*) -1,
                      "[METEO] Error while trying to attach to configuration shared memory")

    CHECK_ERROR_CHILD((shm_dump_ships = shmat(shm_cfg->shm_id_dump_ships, NULL, 0)) == (void*) -1,
                      "[METEO] Error while trying to attach to ships dump shared memory")

    available_ships = shm_cfg->SO_NAVI;
    index_pid_status = malloc(sizeof(int) * shm_cfg->SO_NAVI);

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

    if(shm_cfg->SO_MAELSTORM > 0) {
        while (available_ships) {
            nanosleep_function(shm_cfg->SO_MAELSTORM / 24.0 * shm_cfg->SO_DAY_LENGTH, "[METEO] Generic error in nanosleep");
            index_pid_to_term = (unsigned int) random() % available_ships;
            printf("[METEO] index to kill: %d\n", index_pid_to_term);
            kill(shm_pid_array[index_pid_status[index_pid_to_term] + shm_cfg->SO_PORTI], SIGUSR2);
            index_pid_status[index_pid_to_term] = index_pid_status[--available_ships];
        }
        kill(getppid(), SIGUSR1);
        free(index_pid_status);
    } else {
        while (1) {
            pause();
        }
    }
    return 0;
}

void meteo_sig_handler(int signum) {
    int old_errno = errno;

    switch (signum) {
        case SIGCONT:
            break;
        case SIGALRM:
            if (shm_cfg->SO_SWELL_DURATION > 0) {
                kill(shm_pid_array[random() % shm_cfg->SO_PORTI], SIGUSR1);
            }
            if (shm_cfg->SO_STORM_DURATION > 0) {
                /*TODO: in questo momento c'è race condition sull'update di ships slowed in dump dei porti*/
                kill(shm_pid_array[index_pid_status[random() % available_ships] + shm_cfg->SO_PORTI], SIGUSR1);
            }
            raise(SIGSTOP);
            break;
        case SIGTERM:
            exit(EXIT_SUCCESS);
        default:
            printf("[METEO] Signal: %s\n", strsignal(signum));
            break;
    }
    errno = old_errno;
}
