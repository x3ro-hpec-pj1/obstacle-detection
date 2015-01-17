#define _BSD_SOURCE 1

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <stdbool.h>
#include <unistd.h>

#include "scanner_reader.h"
#include "obstacle_detection.h"
#include "visualization.h"

pthread_t workerThread1; // pointers to control the used worker-thread object
pthread_t workerThread2;
pthread_mutex_t mutexMEM; // mutex for shared memory used by segmentation and for canvas used as visualization
pthread_mutex_t mutexUSB; // mutex for USB-access used to receive measurement data from laserscanner
int framesProcessed1;
int framesProcessed2;

typedef struct od_thread_s {
    FILE *fp;
    pthread_mutex_t *mutexUSB;
    int *framesProcessed;
} od_thread_s;




void *thread_loop(void *ptr);

void init_memory() {
    printf("Initializing memory\n");

    int status;
    status = pthread_mutex_init(&mutexUSB, NULL);
    if(status != 0) {
        fprintf(stderr, "'Could not initialize mutex' near line %d.\n", __LINE__);
        exit(1);
    }

    status = pthread_mutex_lock(&mutexUSB);
    if(status != 0) {
        fprintf(stderr, "'Error locking mutex' near line %d.\n", __LINE__);
        exit(1);
    }

    status = pthread_mutex_unlock(&mutexUSB);
    if(status != 0) {
        fprintf(stderr, "'Error unlocking mutex' near line %d.\n", __LINE__);
        exit(1);
    }
}

void init_threads(FILE *fp) {
    printf("Initializing threads\n");

    od_thread_s *config1 = malloc(sizeof(od_thread_s));
    od_thread_s *config2 = malloc(sizeof(od_thread_s));
    config1->fp = fp;
    config2->fp = fp;
    config1->framesProcessed = &framesProcessed1;
    config2->framesProcessed = &framesProcessed2;
    config1->mutexUSB = &mutexUSB;
    config2->mutexUSB = &mutexUSB;

    int ret;
    ret = pthread_create( &workerThread1, NULL, thread_loop, (void*)config1);
    if(ret) {
        fprintf(stderr,"Error - pthread_create() return code: %d\n",ret);
        exit(EXIT_FAILURE);
    }

    ret = pthread_create( &workerThread2, NULL, thread_loop, (void*)config2);
    if(ret) {
        fprintf(stderr,"Error - pthread_create() return code: %d\n",ret);
        exit(EXIT_FAILURE);
    }
}

void *thread_loop(void *ptr) {
    od_thread_s *config = (od_thread_s*) ptr;
    pthread_t pt = pthread_self();

    obstacle_detection_data* data = obstacle_detection_init_memory();
    char databuffer[SCANNER_SEGMENT_SIZE];
    int bytes_read;

    printf("Initialized thread %d\n", (int) pt);

    while(1) {
        pthread_mutex_lock(config->mutexUSB);
        //printf("enter\n");
        bytes_read = read_scanner_segment(databuffer, config->fp);
        if(bytes_read != 1616) {
            fprintf(stderr, "Read non-data scanner segment. Skipping\n");
            pthread_mutex_unlock(config->mutexUSB);
            continue;
        }
        // printf("exit\n");
        pthread_mutex_unlock(config->mutexUSB);

        evaluate_scanner_segment(data, databuffer);
        detect_obstacle_segments(data);
        do_ransac(data);
        //visualize(data);
        obstacle_detection_zero_memory(data);

        (*config->framesProcessed)++;
    }

    return NULL;
}

int main(int argc, const char *argv[]) {
    setbuf(stdout, NULL); // Disable buffering for stdout

    if(argc != 2) {
        fprintf(stderr, "'Please supply the input file as the only argument' near line %d.\n", __LINE__);
        exit(1);
    }

    printf("\n");

    FILE *fp = scanner_open(argv[1], true);
    //obstacle_detection_initialize_visualization();
    init_memory();
    init_threads(fp);

    printf("Initialization complete\n");

    // pthread_join( workerThread1, NULL);
    // pthread_join( workerThread2, NULL);
    //
    clock_t start = clock(), diff, now;
    long frame_count = 0;
    long last_framecount = 0;
    while(1) {
        usleep(1000000);
        frame_count = framesProcessed1 + framesProcessed2;
        now = clock();
        diff = now - start;
        double elapsed = diff*1.0 / CLOCKS_PER_SEC;
        printf("Throughput: %f MB/s\n", ((frame_count - last_framecount) * 1616.0)/(elapsed)/1024/1024);
        start = now;
        last_framecount = frame_count;

    }

    return 0;
}
