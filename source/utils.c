#include "../headers/utils.h"
#include "math.h"

char* int_to_string(int number) {
    char* s;
    int s_len;

    /* 10^x <= number < 10^(x + 1) */
    if(number > 0) s_len = (int) ((floor(log10(number)) + 2) * sizeof(char));
    else if(number < 0) s_len = (int) ((floor(log10(-number)) + 3) * sizeof(char));
    else s_len = 2;

    s = malloc(s_len);

    snprintf(s, s_len, "%d", number);
    return s;
}

int string_to_int(char *s) {
    char *endptr;
    int val = (int) strtol(s, &endptr, 10);

    if (endptr == s) {
        errno = EINVAL;
    }
    return val;
}

void nanosleep_function(double time, char* str) {
    struct timespec t, rem;
    double d;
    t.tv_nsec = (long) (modf(time, &d) * 1e9);
    t.tv_sec = (long) d;

    while (nanosleep(&t, &rem)) {
        switch (errno) {
            case EINTR:
                t = rem;
                continue;
            default:
                perror(str);
                exit(EXIT_FAILURE);
        }
    }
}

/* ordina in base al peso la merce */
int compare_goods_template(const void *g, const void *g1) {
    goods_template *s_g = (goods_template*) g;
    goods_template *s_g1 = (goods_template*) g1;

    return s_g->tons - s_g1->tons;
}

void print_config(config *cfg) {
    printf("--------------------------\n");
    printf("--------- CONFIG ---------\n");
    printf("--------------------------\n");
    printf("SO_NAVI:        %d\n", cfg->SO_NAVI);
    printf("SO_PORTI:       %d\n", cfg->SO_PORTI);
    printf("SO_MERCI:       %d\n", cfg->SO_MERCI);
    printf("SO_SIZE:        %d\n", cfg->SO_SIZE);
    printf("SO_MIN_VITA:    %d\n", cfg->SO_MIN_VITA);
    printf("SO_MAX_VITA:    %d\n", cfg->SO_MAX_VITA);
    printf("SO_LATO:        %f\n", cfg->SO_LATO);
    printf("SO_SPEED:       %d\n", cfg->SO_SPEED);
    printf("SO_CAPACITY:    %d\n", cfg->SO_CAPACITY);
    printf("SO_BANCHINE:    %d\n", cfg->SO_BANCHINE);
    printf("SO_FILL:        %d\n", cfg->SO_FILL);
    printf("SO_LOADSPEED:   %d\n", cfg->SO_LOADSPEED);
    printf("SO_DAYS:        %d\n", cfg->SO_DAYS);
    printf("SO_STORM_DURATION: %d\n", cfg->SO_STORM_DURATION);
    printf("SO_SWELL_DURATION: %d\n", cfg->SO_SWELL_DURATION);
    printf("SO_MAELSTORM:   %d\n", cfg->SO_MAELSTORM);
    printf("--------------------------\n");
}
