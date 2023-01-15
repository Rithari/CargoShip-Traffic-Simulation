#include "../headers/utils.h"
#include "../headers/common_ipcs.h"
#include "../headers/linked_list.h"

typedef struct {
    int how_many;
    struct node *goods_to_send;
    int port_id;
} route;

void porto_sig_handler(int);
void generate_goods(void);
route* generate_route(void);
void dump_port_data(void);

config  *shm_cfg;
coord   *shm_ports_coords;
goods_template   *shm_goods_template;
int*    shm_goods;
pid_t   *shm_pid_array;
dump_ports  *shm_dump_ports;
dump_goods  *shm_dump_goods;
struct node *head;

int     shm_id_config;
int     id;

int main(int argc, char *argv[]) {
    struct sigaction sa;
    msg_handshake msg;
    route* r;
    msg_goods msg_g;

    if(argc != 3) {
        printf("Incorrect number of parameters [%d]. Exiting...\n", argc);
        exit(EXIT_FAILURE);
    }

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = porto_sig_handler;

    sigaction(SIGCONT, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGUSR2, &sa, NULL);
    sa.sa_flags |= SA_NODEFER;
    sigaction(SIGUSR1, &sa, NULL);
    sigaddset(&sa.sa_mask, SIGUSR1);
    sigaction(SIGALRM, &sa, NULL);

    srandom(getpid());

    /* Attach to the shared memory config by the ID passed as args and attach to all needed variables */
    shm_id_config = string_to_int(argv[1]);
    CHECK_ERROR_CHILD(errno, "[PORTO] Error while trying to convert shm_id_config")
    id = string_to_int(argv[2]);
    CHECK_ERROR_CHILD(errno, "[PORTO] Error while trying to convert id")

    CHECK_ERROR_CHILD((shm_cfg = shmat(shm_id_config, NULL, SHM_RDONLY)) == (void*) -1,
                      "[PORTO] Error while trying to attach to configuration shared memory")
    CHECK_ERROR_CHILD((shm_pid_array = shmat(shm_cfg->shm_id_pid_array, NULL, 0)) == (void*) -1,
                      "[PORTO] Error while trying to attach to pid_array shared memory")
    CHECK_ERROR_CHILD((shm_goods = shmat(shm_cfg->shm_id_goods, NULL, 0)) == (void*) -1,
                      "[PORTO] Error while trying to attach to goods shared memory")
    CHECK_ERROR_CHILD((shm_ports_coords = shmat(shm_cfg->shm_id_ports_coords, NULL, SHM_RDONLY)) == (void*) -1,
                      "[PORTO] Error while trying to attach to ports coordinates shared memory")
    CHECK_ERROR_CHILD((shm_goods_template = shmat(shm_cfg->shm_id_goods_template, NULL, SHM_RDONLY)) == (void*) -1,
                      "[PORTO] Error while trying to attach to goods_template shared memory")
    CHECK_ERROR_CHILD((shm_dump_ports = shmat(shm_cfg->shm_id_dump_ports, NULL, 0)) == (void*) -1,
                      "[PORTO] Error while trying to attach to dump port shared memory")
    CHECK_ERROR_CHILD((shm_dump_goods = shmat(shm_cfg->shm_id_dump_goods, NULL, 0)) == (void*) -1,
                      "[PORTO] Error while trying to attach to dump goods_template shared memory")

    shm_dump_ports[id].dock_total = (int) random() % shm_cfg->SO_BANCHINE + 1;
    CHECK_ERROR_CHILD(semctl(shm_cfg->sem_id_dock, id, SETVAL, shm_dump_ports[id].dock_total),
                      "[PORTO] Error while generating dock semaphore")

    CHECK_ERROR_CHILD(sem_cmd(shm_cfg->sem_id_gen_precedence, 0, -1, 0) < 0,
                      "[PORTO] Error while trying to release sem_id_gen_precedence")

    head = NULL;
    pause();

    while (1) {
        /* Waiting for an initial handshake message from ships. Once it gets one, it starts the response proceedings */
        while (msgrcv(shm_cfg->mq_id_ports_handshake, &msg, sizeof(msg) - sizeof(long), id + 1, 0) < 0) {
            CHECK_ERROR_CHILD(errno != EINTR, "[PORTO] Error while waiting handshake message")
        }
        /*printf("[%d] Received message from [%d]\n", getpid(), msg.response_pid);*/
        msg.mtype = msg.response_pid;

        if (head) {
            r = generate_route();
            msg.how_many = r->how_many;
            msg.response_pid = r->port_id;
            /*ll_print(r->goods_to_send);*/
            while (msgsnd(shm_cfg->mq_id_ships_handshake, &msg, sizeof(msg) - sizeof(long), 0)) {
                CHECK_ERROR_CHILD(errno != EINTR, "[PORTO] Error while sending handshake message")
            }

            while (r->goods_to_send) {
                msg_g.mtype = msg.mtype;
                msg_g.to_add.quantity = r->goods_to_send->element->quantity;
                msg_g.to_add.id = r->goods_to_send->element->id;
                msg_g.to_add.lifespan = r->goods_to_send->element->lifespan;

                while (msgsnd(shm_cfg->mq_id_ships_goods, &msg_g, sizeof(msg_goods) - sizeof(long), 0)) {
                    CHECK_ERROR_CHILD(errno != EINTR, "[PORTO] Error while sending the struct of goods to add")
                }
                __sync_fetch_and_sub(&shm_goods[id * shm_cfg->SO_MERCI + msg_g.to_add.id], msg_g.to_add.quantity);
                __sync_fetch_and_add(&shm_dump_ports[id].good_send, msg_g.to_add.quantity * shm_goods_template[msg_g.to_add.id].tons);
                __sync_fetch_and_sub(&shm_dump_ports[id].good_available, msg_g.to_add.quantity * shm_goods_template[msg_g.to_add.id].tons);
                r->goods_to_send = ll_pop(r->goods_to_send);
            }
            /* Free up the memory allocated */
            ll_free(r->goods_to_send);
            free(r);
            /*printf("[%d] Ok given to [%ld]\n", getpid(), msg.mtype);*/
        } else {
            /* If there are no goods to send, send a message to the ship to tell it to go away */
            msg.how_many = 0;
            msg.response_pid = -1;
            while (msgsnd(shm_cfg->mq_id_ships_handshake, &msg, sizeof(msg) - sizeof(long), 0)) {
                CHECK_ERROR_CHILD(errno != EINTR, "[PORTO] Error while sending handshake message")
            }
        }
    }
}

/* Generate goods and add them to al inked list */
void generate_goods(void) {
    int i;
    int selected_quantity;
    int tons_per_port_offers = shm_cfg->SO_FILL / shm_cfg->SO_DAYS / shm_cfg->GENERATING_PORTS;
    int tons_per_port_request = tons_per_port_offers;
    int is_offering;
    goods to_add;

    tons_per_port_offers += shm_dump_ports[id].ton_in_excess_offers;
    tons_per_port_request += shm_dump_ports[id].ton_in_excess_request;
    shm_dump_ports[id].total_goods_offers += tons_per_port_offers;
    shm_dump_ports[id].total_goods_requested += tons_per_port_request;

    for(i = 0; i < shm_cfg->SO_MERCI; i++) {
        int max_quantity;

        if (((random() & 1) && shm_goods[id * shm_cfg->SO_MERCI + i] == 0) || shm_goods[id * shm_cfg->SO_MERCI + i] > 0) {
            max_quantity = tons_per_port_offers / shm_goods_template[i].tons;
            is_offering = 1;
        } else {
            max_quantity = tons_per_port_request / shm_goods_template[i].tons;
            is_offering = 0;
        }

        if (max_quantity == 0) continue;

        selected_quantity = (int) random() % max_quantity + 1;

        if (is_offering) {
            to_add.lifespan = shm_cfg->CURRENT_DAY + shm_goods_template[i].lifespan;
            to_add.id = i;
            to_add.quantity = selected_quantity;
            shm_goods[id * shm_cfg->SO_MERCI + i] += selected_quantity;
            __sync_fetch_and_add(&shm_dump_goods[i].good_in_port, selected_quantity * shm_goods_template[i].tons);
            __sync_fetch_and_add(&shm_dump_ports[id].good_available, selected_quantity * shm_goods_template[i].tons);
            tons_per_port_offers -= (selected_quantity * shm_goods_template[i].tons);
            head = ll_add(head, &to_add);
        } else {
            shm_goods[id * shm_cfg->SO_MERCI + i] -= selected_quantity;
            tons_per_port_request -= (selected_quantity * shm_goods_template[i].tons);
        }
    }
    shm_dump_ports[id].ton_in_excess_offers = tons_per_port_offers;
    shm_dump_ports[id].ton_in_excess_request = tons_per_port_request;
    shm_dump_ports[id].total_goods_offers -= tons_per_port_offers;
    shm_dump_ports[id].total_goods_requested -= tons_per_port_request;
}

/* Generate a route for a ship by selecting a random port
 * as a starting point, then iterating through all other ports to find the best route based on the goods available
 * in each port, and the goods that the ship currently has and wants to trade.
*/
route* generate_route(void) {
    int i, j, k, min_val;
    int best_route = -1;
    int best_tons_available = shm_cfg->SO_CAPACITY;
    int ship_tons_available;
    int *tmp_goods_to_get = malloc(sizeof(int) * shm_cfg->SO_MERCI);
    int *goods_to_get = malloc(sizeof(int) * shm_cfg->SO_MERCI);
    int offset;
    struct node *cur;
    struct node *sublist;
    goods to_add;
    route *r = malloc(sizeof(route));

    for(k = 0, i = (int) random() % shm_cfg->SO_PORTI; k < shm_cfg->SO_PORTI && best_tons_available > shm_goods_template[0].tons; k++, i = (i + 1) % shm_cfg->SO_PORTI) {
        if(id == i) continue;

        for (j = 0; j < shm_cfg->SO_MERCI && shm_goods[i * shm_cfg->SO_MERCI + j] >= 0; j++);

        /* If there are no requests do not iterate */
        if (j == shm_cfg->SO_MERCI) continue;
        else offset = j;

        memset(tmp_goods_to_get, 0, sizeof(int) * shm_cfg->SO_MERCI);

        for(j = shm_cfg->SO_MERCI - 1, ship_tons_available = shm_cfg->SO_CAPACITY; j >= 0 && ship_tons_available > shm_goods_template[offset].tons; j--) {
            if(shm_goods[id * shm_cfg->SO_MERCI + j] > 0 && shm_goods[i * shm_cfg->SO_MERCI + j] < 0 && ship_tons_available / shm_goods_template[j].tons) {
                /* Total amount one can fit inside the ship */
                min_val = min(min(shm_goods[id * shm_cfg->SO_MERCI + j], -shm_goods[i * shm_cfg->SO_MERCI + j]),
                              ship_tons_available / shm_goods_template[j].tons);

                if (!min_val) continue;

                tmp_goods_to_get[j] = min_val;
                ship_tons_available -= min_val * shm_goods_template[j].tons;
            }
        }

        if (ship_tons_available < best_tons_available) {
            best_route = i;
            best_tons_available = ship_tons_available;
            memcpy(goods_to_get, tmp_goods_to_get, sizeof(int) * shm_cfg->SO_MERCI);
        }
    }
    /* Selects a random starting port and iterates through all other ports. For each port, it checks if the
     * port has any goods that the ship wants and the ship has any goods that the port wants. If so, it calculates
     * the total amount of goods that can fit in the ship and adds it to the temporary array of goods to get.
     * It then compares the total amount of goods that can be transported by the ship with the current best route
     * and updates the best route and goods to get if the current route is better.
    */
    sublist = NULL;

    if(best_tons_available < shm_cfg->SO_CAPACITY) {
        int how_many;

        while (head && head->element->lifespan < shm_cfg->CURRENT_DAY) {
            /* update dumps */
            /*printf("[%d] Port lost good :C\n", getpid());*/
            __sync_fetch_and_sub(&shm_goods[id * shm_cfg->SO_MERCI + head->element->id], head->element->quantity);
            __sync_fetch_and_sub(&shm_dump_ports[id].good_available, head->element->quantity * shm_goods_template[head->element->id].tons);
            __sync_fetch_and_add(&shm_dump_goods[head->element->id].good_expired_in_port, head->element->quantity * shm_goods_template[head->element->id].tons);

            head = ll_pop(head);
        }

        for (i = 0, how_many = 0; i < shm_cfg->SO_MERCI; i++) {
            if (!goods_to_get[i]) continue;

            cur = head;

            while (goods_to_get[i] > 0 && cur) {

                if (cur->element->id == i) {
                    to_add.lifespan = cur->element->lifespan;
                    to_add.id = i;
                    if (cur->element->quantity > goods_to_get[i]) {
                        to_add.quantity = goods_to_get[i];
                        cur->element->quantity -= goods_to_get[i];
                        goods_to_get[i] = 0;
                    } else {
                        to_add.quantity = cur->element->quantity;
                        head = ll_remove_by_id(head, i);
                        goods_to_get[i] -= to_add.quantity;
                        cur = head;
                    }
                    sublist = ll_add(sublist, &to_add);
                    how_many++;
                } else cur = cur->next;
            }
        }
        r->how_many = how_many;
        r->port_id = best_route;
        r->goods_to_send = sublist;
        /*printf("Route found: from %d to %d with %d tons available to exchange\n", id, best_route, (shm_cfg->SO_CAPACITY - best_tons_available));*/
    } else {
        r->how_many = 0;
        r->goods_to_send = NULL;
        r->port_id = -1;
    }
    free(goods_to_get);
    free(tmp_goods_to_get);

    return r;

    /*
     * If the best route is found and there is a better amount of goods that can be transported than
     * the ship's capacity, the function updates the shared memory for the goods in the ports, creating a
     * linked list of goods to send and populating a route structure with the destination port, the amount of goods
     * to be transported, and the linked list of goods. It then frees the memory used for the temporary arrays
     * and returns the route structure.
     */
}

void dump_port_data(void) {
    shm_dump_ports[id].dock_available = semctl(shm_cfg->sem_id_dock, id, GETVAL);
}

void porto_sig_handler(int signum) {
    int old_errno = errno;

    switch (signum) {
        case SIGTERM:
            /*ll_print(head);*/
            dump_port_data();
            ll_free(head);
            exit(EXIT_SUCCESS);
        case SIGCONT:
            if (shm_pid_array[id] < 0 && shm_cfg->GENERATING_PORTS > 0) {
                generate_goods();
                shm_pid_array[id] = -shm_pid_array[id];
            }
            break;
        case SIGINT:
            ll_free(head);
            exit(EXIT_FAILURE);
        case SIGALRM:
            dump_port_data();
            /*ll_print(head);*/
            if (shm_pid_array[id] < 0 && shm_cfg->GENERATING_PORTS > 0) {
                generate_goods();
                shm_pid_array[id] = -shm_pid_array[id];
            }
            while (sem_cmd(shm_cfg->sem_id_gen_precedence, 0, -1, 0)) {
                CHECK_ERROR_CHILD(errno != EINTR,
                                  "[PORTO] Error while trying to release sem_id_gen_precedence")
            }
            break;
        case SIGUSR1:
            /* swell occurred */
            /*printf("[PORTO] SWELL: %d\n", getpid());*/
            shm_dump_ports[id].on_swell = 1;
            sleep_ns(shm_cfg->SO_SWELL_DURATION / 24.0 * shm_cfg->SO_DAY_LENGTH,
                     "Generic error while sleeping because of the swell");
            shm_dump_ports[id].on_swell = 0;
            /*printf("[PORTO] END SWELL: %d\n", getpid());*/
            break;
        case SIGUSR2:
            break;
        default:
            /*printf("[PORTO] Signal: %s\n", strsignal(signum));*/
            break;
    }

    errno = old_errno;
}
