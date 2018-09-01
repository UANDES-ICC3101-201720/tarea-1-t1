#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <limits.h>
#include "types.h"
#include "const.h"
#include "util.h"

int serial_binsearch(int *array, int target, int left, int right) {
    int middle = (left + right) / 2;
    if (array[middle] == target) {
        return 1;
    } else if (target < array[middle]) {
        return binsearch(array, target, left, middle - 1);
    } else {
        return binsearch(array, target, middle + 1, right);
    }
}

// TODO: implement
int parallel_binsearch(int *array, int target, int cores) {
    return 0;
}

int main(int argc, char** argv) {
    struct timespec start_serial, finish_serial, start_parallel, finish_parallel;
    double elapsed_serial = 0;
    double elapsed_parallel = 0;

    printf("[binsearch] Starting up...\n");

    int cores = sysconf(_SC_NPROCESSORS_ONLN);

    /* Get the number of CPU cores available */
    printf("[binsearch] Number of cores available: '%ld'\n",
           sysconf(_SC_NPROCESSORS_ONLN));

    /* parse arguments with getopt */

    int tvalue = 3;
    int evalue = 1;
    int pvalue = 1000;

    int index;
    int c;

    opterr = 0;


    while ((c = getopt (argc, argv, "T:E:P:")) != -1)
        switch (c)
        {
            case 'T':
                tvalue = atoi(optarg);
                break;
            case 'E':
                evalue = atoi(optarg);
                break;
            case 'P':
                pvalue = atoi(optarg);
                break;
            case '?':
                if (optopt == 'T')
                    fprintf (stderr, "Option -%c requires an argument.\n", optopt);
                else if (optopt == 'E')
                    fprintf (stderr, "Option -%c requires an argument.\n", optopt);
                else if (optopt == 'P')
                    fprintf (stderr, "Option -%c requires an argument.\n", optopt);
                else if (isprint (optopt))
                    fprintf (stderr, "Unknown option `-%c'.\n", optopt);
                else
                    fprintf (stderr,
                             "Unknown option character `\\x%x'.\n",
                             optopt);
                return 1;
            default:
                abort ();
        }

        //printf ("T = %d, E = %d, P = %d\n", tvalue, evalue, pvalue);
        //for (index = optind; index < argc; index++)
        //    printf ("Non-option argument %s\n", argv[index]);


    /* Start datagen as a child process. */

    int child = fork();
    if (child < 0) {
        fprintf(stderr, "forking a child failed\n");
        exit(-1);
    } else if (child == 0) { // Child process
        close(STDOUT_FILENO); // Close stdout for datagen
        char *myargs[2];
        myargs[0] = strdup("./datagen");
        myargs[1] = NULL;
        printf("Imachild");
        execvp(myargs[0], myargs);
        printf("Imachild2");
//        exit(1);
    } else { // parent
        sleep(1); // wait till child is up and running
    }

    /* Allocate memory for the data set array according with size T */
    UINT readvalues = 0;
    size_t numvalues = pow(10, tvalue);
    size_t readbytes = 0;

    UINT *readbuf = malloc(sizeof(UINT) * numvalues);

    // Begin string to datagen
    char data[10];
    strcpy(data,"BEGIN S ");
    char t[2];
    sprintf(t, "%d", tvalue);
    strcat(data, t);

    // Create socket
    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("[binsearch] Error creating socket.\n");
        exit(-1);
    }
    // Set address pointing DSOCKET_PATH (i.e. /tmp/dg.sock)
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, DSOCKET_PATH, sizeof(addr.sun_path) - 1);

    // Connect to datagen
    if (connect(fd, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
        perror("[binsearch] Connection error.\n");
        exit(-1);
    }

    // Send request to datagen
    if (write(fd, data, strlen(data)) == -1) {
        perror("[binsearch] Sending message error.\n");
        exit(-1);
    }

    // Read stream (by CLAUDIO ALVAREZ GOMEZ)
    while (readvalues < numvalues) {
        /* read the bytestream */
        readbytes = read(fd, readbuf + readvalues, sizeof(UINT) * 1000);
        readvalues += readbytes / 4;
    }


    // Send END to datagen
    if (write(fd, DATAGEN_END_CMD, strlen(DATAGEN_END_CMD)) == -1) {
        perror("[binsearch] Sending END message error.\n");
        exit(-1);
    }

    // Experiments loop
    for (int i = 0; i < evalue; ++i) {

        /* SERIAL: Get the wall clock time at start */
        clock_gettime(CLOCK_MONOTONIC, &start_serial);

        /* Burn CPU time */
        for (int i = 0; i<INT_MAX/2; i++);

        /* SERIAL: Get the wall clock time at finish */
        clock_gettime(CLOCK_MONOTONIC, &finish_serial);



        /* Parallel: Get the wall clock time at start */
        clock_gettime(CLOCK_MONOTONIC, &start_parallel);

        /* Burn CPU time */
        for (int i = 0; i<INT_MAX/2; i++);

        /* Parallel: Get the wall clock time at finish */
        clock_gettime(CLOCK_MONOTONIC, &finish_parallel);



        /* Calculate serial time elapsed */
        elapsed_serial = (finish_serial.tv_sec - start_serial.tv_sec);
        elapsed_serial += (finish_serial.tv_nsec - start_serial.tv_nsec) / 1000000000.0;
        /* Calculate parallel time elapsed */
        elapsed_parallel = (finish_parallel.tv_sec - start_parallel.tv_sec);
        elapsed_parallel += (finish_parallel.tv_nsec - start_parallel.tv_nsec) / 1000000000.0;

        /* Print the time elapsed (in seconds) */
        printf("Serial: %lf\n, parallel: %lf\n", elapsed_serial, elapsed_parallel);

    }





    free(readbuf);
    exit(0);
}