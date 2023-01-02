#ifndef PROGETTOSO_MASTER_H
#define PROGETTOSO_MASTER_H

#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/stat.h>


#ifdef NDEBUG
#define PATH_NAVE   "release/nave.out"
    #define PATH_PORTO  "release/porto.out"
    #define PATH_MASTER "release/master.out"
    #define PATH_METEO  "release/meteo.out"
#else
#define PATH_NAVE   "debug/nave.out"
#define PATH_PORTO  "debug/porto.out"
#define PATH_MASTER "debug/master.out"
#define PATH_METEO  "debug/meteo.out"
#endif


typedef struct {
    int     id;
    int     tons;
    int     lifespan;
} goods;

typedef struct {
    double  x;
    double  y;
} coord;

typedef struct {
    int     id;
    int     state;
} dump_goods;

typedef struct {
    int    id;
    int    good_available;
    int    good_send;
    int    good_received;
    int    dock_total;
    int    dock_available;
    int     on_swell;
} dump_ports;

typedef struct {
    int     ships_with_cargo_en_route;
    int     ships_without_cargo_en_route;
    int     ships_being_loaded_unloaded;
    int     ships_slowed;
    int     ships_sunk;
} dump_ships;

typedef struct {
    long mtype;
    int response_pid;
} msg_handshake;

typedef struct {
    int     CURRENT_DAY;
    int     SO_NAVI;
    int     SO_PORTI;
    int     SO_MERCI;
    int     SO_SIZE;
    int     SO_MIN_VITA;
    int     SO_MAX_VITA;
    double  SO_LATO;
    int     SO_SPEED;
    int     SO_CAPACITY;
    int     SO_BANCHINE;
    int     SO_FILL;
    int     SO_LOADSPEED;
    int     SO_DAYS;
    int     SO_DAY_LENGTH;
    int     SO_STORM_DURATION;
    int     SO_SWELL_DURATION;
    int     SO_MAELSTORM;
    int     shm_id_goods_template;
    int     shm_id_ports_coords;
    int     shm_id_pid_array;
    int     shm_id_dump_ports;
    int     shm_id_dump_ships;
    int     shm_id_dump_goods;
    int     mq_id_request;
    int     mq_id_ports_handshake;
    int     mq_id_ships_handshake;
    int     sem_id_gen_precedence; /* semaphore used to manage the general precedence */
    int     sem_id_dock;
    int     sem_id_dump_mutex;
} config;

#endif /*PROGETTOSO_MASTER_H*/
