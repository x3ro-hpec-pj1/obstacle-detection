#include <pthread.h>
#include <stdio.h> /* for fprintf and stderr */
#include <stdlib.h> /* for exit */

#include "scanner_reader.h"



// threshold to decide new or same segment is dynamically calculated by this factor
const float THRESHOLD_FACTOR = 0.033f; // 16.5mm divided by 500mm

const float RESOLUTION = 0.3515625f;  // 360 divided by 1024 degrees

// draw laserscanner at this position
const float X_CENTER = 1001.0f;
const float Y_CENTER = 308.0f;

pthread_t workerThread1; // pointers to control the used worker-thread object
pthread_t workerThread2;
pthread_mutex_t mutexMEM; // mutex for shared memory used by segmentation and for canvas used as visualization
pthread_mutex_t mutexUSB; // mutex for USB-access used to receive measurement data from laserscanner

char *startScanMScommand = NULL;

int NearestSteps[128]; // index = Obstacle-ID, integer-value = Nearest-Step of Obstacle
int FirstSteps[128]; // index = Obstacle-ID, integer-value = First-Step of Obstacle
int LastSteps[128]; // index = Obstacle-ID, integer-value = Last-Step of Obstacle

#define DISTANCE_VALUE_COUNT 512
int distance[DISTANCE_VALUE_COUNT]; // 18 bit decoded distance-values in millimeter for each measurement step

int timestamp = 0; // 24 bit timestamp received from laserscanner



void initMemory() {
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

    timestamp = -1;
    for(int i = 0; i < DISTANCE_VALUE_COUNT; i++) {
        distance[i] = 0; // initialize array of distances
    }

    status = pthread_mutex_unlock(&mutexMEM);
    if(status != 0) {
        fprintf(stderr, "'Error unlocking mutex' near line %d.\n", __LINE__);
        exit(1);
    }
}


void run() {
    // stores USB-datagram one for each thread, not in shared memory region
    char databuffer[SCANNER_SEGMENT_SIZE];

    float g = 90; // start angle from +90 till -90 degrees
    float l = 0.0f; // length of line to be drawn on the surface

    long timing = 0; // timestamp to measure time of segmentation-cycle
    int bytesreaded = 0; // return value of USB read routine
    int offbyone = 0; // needed to ensure error-tolerance while reading USB packets

    while(1) {

    }
}

/**
 * Will modify the segment buffer!
 *
 */

/**
 * Parses the given scanner segment, extracting the distance values and writing
 * them into the given distance array.
 *
 * Note: This will modify the passed segment buffer!
 *
 * @param  segment
 * @param  distances
 * @return The SICP2.0 timestamp of the parsed scanner segment.
 */
int evaluate_scanner_segment(char *segment, int *distances) {
    int timestamp;

    timestamp = segment[0] - 0x30;
    timestamp <<= 6;
    timestamp &= 0xFFFFC0; // clear six-most MSB bits according SCIP 2.0
    timestamp |= segment[1] - 0x30;
    timestamp <<= 6;
    timestamp &= 0xFFFFC0;
    timestamp |= segment[2] - 0x30;
    timestamp <<= 6;
    timestamp &= 0xFFFFC0;
    timestamp |= segment[3] - 0x30;

    int idx = 0;
    // Start processing data after initial 6 bytes of header, including LF
    // Data is comprised of 64 byte chunks, a 1-byte sum and a line feed
    for(int i = 6; i < 1550; i += 66) {
        for(int j = 0; j < 64; j++) {
            segment[idx] = segment[i+j];
            idx++;
        }
    }

    // Now we have a continous stream of scanner data, without header, sums and
    // linefeeds. We now decode the extracted distance values.
    // 1536 bytes is the raw data packet size.
    for(idx = 0; idx < 1536; idx += 3){
        distances[idx/3] = 0;
        distances[idx/3] = segment[idx] - 0x30;
        distances[idx/3] <<= 6;
        distances[idx/3] &= 0xFFFFC0; // clear six-most MSB bits
        distances[idx/3] |= segment[idx+1] - 0x30;
        distances[idx/3] <<= 6;
        distances[idx/3] &= 0xFFFFC0;
        distances[idx/3] |= segment[idx+2] - 0x30;
    }

     return timestamp;
}



int main(int argc, const char *argv[]) {
    int status;

    if(argc != 2) {
        fprintf(stderr, "'Please supply the input file as the only argument' near line %d.\n", __LINE__);
        exit(1);
    }

    status = load_input_file(argv[1]);
    if(status != 0) {
        fprintf(stderr, "'Could not load input file' near line %d.\n", __LINE__);
        exit(1);
    }

    initMemory();
    printf("done11\n");

    char databuffer[SCANNER_SEGMENT_SIZE];
    int bytes_read;

    while(1) {
        bytes_read = read_scanner_segment(databuffer);
        if(bytes_read != 1616) {
            fprintf(stderr, "Read non-data scanner segment. Skipping\n");
            continue;
        }

        int timestamp = evaluate_scanner_segment(databuffer, distance);
        printf("timestamp: %d\n", timestamp);
        break;
    }
    return 0;
}
