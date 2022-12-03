#include "../headers/utils.h"
#include "../headers/master.h"
#include "../headers/common_ipcs.h"

/*
Richiesta di merci[]  | MQ di richieste universale tra processi: (dobbiamo sapere il tipo, la quantità e il porto di appartenenza)
Int Lifespan -1 per le richieste perché non scadono e sono l’unico elemento che distingue le due MQ oltre ad essere due MQ diverse
Array / struct di tratte disponibili (caricata dalla funzione apposita)

Funzione per caricare le proprie domande e offerte nelle rispettive MQ
Funzione/i per la creazione di tratte()
Funzione/i per la comunicazione con le queue()
Banchina: gestita come una risorsa condivisa protetta da un semaforo (n_docks)*/



void porto_sig_handler(int);

/*L'hadler riceve il segnale di creazione delle merci e invoca la funzione designata --> "start_of_goods_generation"*/

goodsList start_of_goods_generation(config *shm_cfg);  /*IMPORTANTE la lista non mi ritorna nel main ma nell'handler() ??LISTAGLOBALE??*/

goodsOffers goodsOffers_struct_generation(int id, int ton, int lifespan);
goodsRequests goodsRequest_struct_generation(int id, int ton, pid_t affiliated);
goodsList goodsOffers_generator(int offersValue, int goodsSet);
void goodsRequest_generator(int requestValue);

/*L'hadler riceve il segnale di creazione delle merci e invoca la funzione designata*/

int main(void) {

    config *shm_cfg;
    int shm_id;
    int n_docks;

    srandom(getpid());

    if((shm_id = shmget(KEY_CONFIG, sizeof(*shm_cfg), 0600)) < 0) {
        perror("Error during porto->shmget()");
        exit(EXIT_FAILURE);
    }

    if((shm_cfg = shmat(shm_id, NULL, SHM_RDONLY)) == (void*) -1) {
        perror("Error during porto->shmat()");
        exit(EXIT_FAILURE);
    }

    n_docks = (int)(random()%(shm_cfg->SO_BANCHINE)+1) ;
    printf("Il porto: %d, ha: %d banchine\n", getpid(), n_docks);
    /* n_ docks dovranno essere gestite come risorsa condivisa protetta da un semaforo */

    goodsList myOffers = start_of_goods_generation(shm_cfg);

    return 0;
}

/*creazione struct*/

goodsList start_of_goods_generation(config *shm_cfg) {
    int maxFillValue = (int)((shm_cfg->SO_FILL / shm_cfg->SO_DAYS) / shm_cfg->SO_PORTI);
    int offersValue = (int)(random()%maxFillValue);
    int requestValue = maxFillValue - offersValue;
    /* parte di codice in cui capisco cosa chiedo e cosa sto già offrendo */
    int goodsSet[shm_cfg->SO_MERCI];
    int i = 0;
    for(; i<shm_cfg->SO_MERCI; i++) {
        goodsSet[i] = (int)(random()%3);  /*con: 0==Nulla , 1==Richiesta , 2==offerta*/
    }
    goodsOffers_generator(offersValue, goodsSet/*, ipotetico array di lifespan*/); /* crea la goodslist delle offerte */

    /*goodsRequest_generator(requestValue);  pusha in MQ le request */
}

goodsOffers goodsOffers_struct_generation(int id, int ton, int lifespan) {
    goodsOffers newOffers;
    newOffers.id = id;
    newOffers.ton = ton;
    newOffers.lifespan = lifespan;
    return newOffers;
}
goodsRequests goodsRequest_struct_generation(int id, int ton, pid_t affiliated) {

    /*Creazione di Request*/

}

goodsList goodsOffers_generator(int offersValue, int *goodsSet) {

    goodsOffers nGx = goodsOffers_struct_generation(id, ton, lifespan);
    /*Return della testa della lista di goods*/


}


void goodsRequest_generator(int requestValue, int *goodsSet) {

    goodsRequests nGx = goodsRequest_struct_generation(id , ton, getpid());
    /*Nessun return, la goods viene pusciata in MQ*/

}