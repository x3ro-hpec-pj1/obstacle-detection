#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "scanner_reader.h"
#include "obstacle_detection.h"

pthread_t workerThread1; // pointers to control the used worker-thread object
pthread_t workerThread2;
pthread_mutex_t mutexMEM; // mutex for shared memory used by segmentation and for canvas used as visualization
pthread_mutex_t mutexUSB; // mutex for USB-access used to receive measurement data from laserscanner

char *startScanMScommand = NULL;


void init_memory() {
    int status;
    status = pthread_mutex_init(&mutexMEM, NULL);
    if(status != 0) {
        fprintf(stderr, "'Could not initialize mutex' near line %d.\n", __LINE__);
        exit(1);
    }

    status = pthread_mutex_lock(&mutexMEM);
    if(status != 0) {
        fprintf(stderr, "'Error locking mutex' near line %d.\n", __LINE__);
        exit(1);
    }

    obstacle_detection_init_memory();

    status = pthread_mutex_unlock(&mutexMEM);
    if(status != 0) {
        fprintf(stderr, "'Error unlocking mutex' near line %d.\n", __LINE__);
        exit(1);
    }
}

int main(int argc, const char *argv[]) {
    setbuf(stdout, NULL); // Disable buffering for stdout

    if(argc != 2) {
        fprintf(stderr, "'Please supply the input file as the only argument' near line %d.\n", __LINE__);
        exit(1);
    }

    printf("\nInitializing\n");

    FILE *fp = scanner_open(argv[1]);
    init_memory();

    char databuffer[SCANNER_SEGMENT_SIZE];
    int bytes_read;

    printf("Initialization complete\nFrames processed: ");

    while(1) {
        bytes_read = read_scanner_segment(databuffer, fp);
        if(bytes_read != 1616) {
            fprintf(stderr, "Read non-data scanner segment. Skipping\n");
            continue;
        }

        evaluate_scanner_segment(databuffer);
        detect_obstacle_segments();
        visualize();

        printf(".");
    }
    return 0;
}
