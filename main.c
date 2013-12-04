#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "scanner_reader.h"
#include "obstacle_detection.h"

const float RESOLUTION = 0.3515625f;  // 360 divided by 1024 degrees

// draw laserscanner at this position
const float X_CENTER = 1001.0f;
const float Y_CENTER = 308.0f;

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


void run() {
    // stores USB-datagram one for each thread, not in shared memory region
    // char databuffer[SCANNER_SEGMENT_SIZE];

    // float g = 90; // start angle from +90 till -90 degrees
    // float l = 0.0f; // length of line to be drawn on the surface

    // long timing = 0; // timestamp to measure time of segmentation-cycle
    // int bytesreaded = 0; // return value of USB read routine
    // int offbyone = 0; // needed to ensure error-tolerance while reading USB packets

    // while(1) {

    // }
}





int main(int argc, const char *argv[]) {
    if(argc != 2) {
        fprintf(stderr, "'Please supply the input file as the only argument' near line %d.\n", __LINE__);
        exit(1);
    }

    FILE *fp = fopen(argv[1], "rb");
    if(fp == NULL) {
        fprintf(stderr, "'Could not open file %s' near line %d.\n", argv[1], __LINE__);
        exit(1);
    }

    init_memory();

    char databuffer[SCANNER_SEGMENT_SIZE];
    int bytes_read;


    while(1) {
        bytes_read = read_scanner_segment(databuffer, fp);
        if(bytes_read != 1616) {
            fprintf(stderr, "Read non-data scanner segment. Skipping\n");
            continue;
        }

        int timestamp = evaluate_scanner_segment(databuffer);
        printf("timestamp: %d\n", timestamp);
        int obid = detect_obstacle_segments();
        printf("obid: %d\n", obid);

    }
    return 0;
}
