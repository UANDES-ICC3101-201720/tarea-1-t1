#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdbool.h>
#include <string.h>
#include "types.h"
#include "const.h"

int cmpfunc (const void * a, const void * b) {
    const unsigned int ua = *(const unsigned int*) a;
    const unsigned int ub = *(const unsigned int*) b;

    if (ua == ub) {
        return 0;
    }
    else if (ua < ub){
        return -1;
    }
    else {
        return 1;
    }
}

UINT* generate_values(UINT T, bool sorted) {
    size_t size = pow(10, T);
    UINT* valuebuff = malloc(sizeof(UINT) * size);

    FILE* rand = fopen("/dev/urandom", "r");

    size_t numvalues = 0;
    size_t remaining_values = size;

    while (numvalues < size) {
        size_t bread = fread(valuebuff+numvalues, sizeof(UINT), min(remaining_values, 1000), rand);
        numvalues += bread;
        remaining_values -= bread;
    }

    fclose(rand);

    if (sorted) {
        printf("[datagen] sorting values.\n");
        qsort(valuebuff, size, sizeof(UINT), cmpfunc);
    }

    return valuebuff;
}

int main(int argc, char** argv) {
    struct sockaddr_un addr;
    char buf[100];
    int fd,cl,rc;

    if ( (fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("[datagen] Error creating socket.\n");
        exit(-1);
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;

    strncpy(addr.sun_path, DSOCKET_PATH, sizeof(addr.sun_path)-1);
    unlink(DSOCKET_PATH);

    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("[datagen] Bind error.\n");
        exit(-1);
    }

    if (listen(fd, 5) == -1) {
        perror("[datagen] Error listening for incoming connections.\n");
        exit(-1);
    }

    while (1) {
        if ( (cl = accept(fd, NULL, NULL)) == -1) {
            perror("[datagen] Error accepting incomming connection.\n");
            continue;
        }

        while ( (rc=read(cl,buf,sizeof(buf))) > 0) {
            char cmd[6], sflag;
            int tvalue;
            int toks = sscanf(buf, DATAGEN_BEGIN_CMD_FMT, cmd, &sflag, &tvalue);

            if (toks == 3 && strstr(cmd, "BEGIN") != NULL) {
                printf("[datagen] Beginning value generation.\n");
                bool sorted = sflag == 'S';

                if (tvalue < 3 || tvalue > 8) {
                    perror("[datagen] Invalid T value! Aborting.\n");
                    continue;
                }

                UINT* pvalues = generate_values(tvalue, sorted);

                printf("[datagen] post value generation and sorting.\n");

                size_t numvalues = 0;
                int remaining_values = pow(10, tvalue);

                write(cl, DATAGEN_OK_RESPONSE, strlen(DATAGEN_OK_RESPONSE));

                printf("[datagen] wrote OK response to socket.\n");

                while (remaining_values > 0) {
                    size_t bwritten = write(cl, pvalues+numvalues, min(remaining_values, 1000) * sizeof(UINT));
                    size_t vwritten = bwritten / sizeof(UINT);
                    numvalues += vwritten;
                    remaining_values -= vwritten;
                    printf("[datagen] wrote '%lu' bytes to socket. '%d' remaining bytes.\n",
                           vwritten, remaining_values);
                }

                free(pvalues);
            }
            else if (strstr(cmd, DATAGEN_END_CMD) != NULL) {
                printf("[datagen] Now exiting.\n");
                close(fd);
                exit(0);
            }
            else {
                perror("[datagen] Command not understood!\n");
            }
        }
        if (rc == -1) {
            perror("[datagen] error reading.\n");
            exit(-1);
        }
        else if (rc == 0) {
            continue;
        }
    }
}