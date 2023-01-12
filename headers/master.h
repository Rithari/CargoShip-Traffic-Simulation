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


/* Constraints based on the build-type. These are passed by makefile targets and used mainly in process executions */
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
} goods_template;

typedef struct {
    int     id;
    int     quantity;
    int     lifespan;
} goods;

typedef struct {
    double  x;
    double  y;
} coord;

/* Structs used to track informational dumps */
typedef struct {
    int     id;
    int     good_in_port;
    int     good_on_ship;
    int     good_delivered;
    int     good_expired_in_port;
    int     good_expired_on_ship;
} dump_goods;

typedef struct {
    int     good_available;
    int     good_send;
    int     good_received;
    int     dock_total;
    int     dock_available;
    int     on_swell;
    int     ton_in_excess;
} dump_ports;

typedef struct {
    int     with_cargo_en_route;
    int     without_cargo_en_route;
    int     being_loaded_unloaded;
    int     slowed;
    int     sunk;
} dump_ships;

/* Handshakes
 * Figuratively speaking, very similar to the concept of handshaking in telecommunications.
 * Used to synchronize between ships and ports.
 * The how_many field is used to know how many messages will be sent and received
*/
typedef struct {
    long mtype;
    int response_pid;
    int how_many;
} msg_handshake;

/* to_add is the goods lot that will be added */
typedef struct {
    long mtype;
    goods to_add;
} msg_goods;

/* Configuration struct, holds all variables needed in the brief as well as
 * IPC objects' IDs and a few user-defined variables
*/
typedef struct {
    /* CONFIG VARIABLES */
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

    /* IPC OBJECTS' IDS */
    int     shm_id_goods_template;
    int     shm_id_ports_coords;
    int     shm_id_pid_array;
    int     shm_id_goods;
    int     shm_id_dump_ports;
    int     shm_id_dump_ships;
    int     shm_id_dump_goods;
    int     mq_id_ports_handshake;
    int     mq_id_ships_handshake;
    int     mq_id_ships_goods;
    int     sem_id_gen_precedence; /* semaphore used to manage the general precedence of process creation */
    int     sem_id_dock;
    int     sem_id_dump_mutex;

    /* USER-DEFINED VARIABLES */
    int     GENERATING_PORTS;
} config;

#endif /*PROGETTOSO_MASTER_H*/
