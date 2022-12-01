#include "../headers/master.h"
#include "../headers/utils.h"
#include "../headers/common_ipcs.h"

int main(int argc, char *argv[]) {
    double pos[2];

    if(argc != 2) {
        /* random position */
        pos[0] = (double)random() / RAND_MAX;
        pos[1] = (double)random() / RAND_MAX;
    }
    else {
        /* position from command line */
        pos[0] = strtod(argv[1], NULL);
        pos[1] = strtod(argv[2], NULL);
    }
    return 0;
}
