#include "../headers/master.h"
#include "../headers/utils.h"
#include "../headers/common_ipcs.h"

#define BUFFER_SIZE 128

const goods *goods_types;

void master_sig_handler(int signum);

void initialize_so_vars(config *cfg, char* path_cfg_file) {
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

int main(int argc, char** argv) {
    struct sigaction sa;
    config *shm_cfg;
    int shm_id;
    char *arg[] = {
            PATH_NAVE,
            NULL,
    };

    /*???????????????????????????????????????????????????????????????????????????????????????????*/
    system("ls");
    /*???????????????????????????????????????????????????????????????????????????????????????????*/

    printf("PARENT KEY: %d", KEY_CONFIG);

    if((shm_id = shmget(KEY_CONFIG, sizeof(*shm_cfg), IPC_CREAT | IPC_EXCL | 0600)) < 0) {
        if(errno == EEXIST) {
            shm_id = shmget(KEY_CONFIG, sizeof(*shm_cfg), 0600);
            shmctl(shm_id, IPC_RMID, NULL);
        }

        printf("Error during master->shmget()");

        exit(EXIT_FAILURE);
    }

    if((shm_cfg = shmat(shm_id, NULL, 0)) == (void*) -1) {
        perror("Error during master->shmat()");
        exit(EXIT_FAILURE);
    }

    initialize_so_vars(shm_cfg, PATH_CONFIG);
    print_config(shm_cfg);


    sa.sa_handler = master_sig_handler;
    sigaction(SIGINT, &sa, NULL);

    system("ls");

    if(!fork()) {
        execv(PATH_NAVE, arg);
        perror("execv has failed");
        exit(EXIT_FAILURE);
    }

    while (wait(NULL) > 0)
    {
        /* code */
    }

    /*??????????????????????????????????????????????????????????????????????????????????????????????????????*/
    shmctl(shm_id, IPC_RMID, NULL);
    /*??????????????????????????????????????????????????????????????????????????????????????????????????????*/

    printf("Ho terminato!\n");


    return 0;
}

void master_sig_handler(int signum) {
    switch (signum)
    {
        case SIGINT:

            break;

        default:
            break;
    }
}