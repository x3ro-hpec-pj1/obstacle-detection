#ifndef OBSTACLEDETECTION_H_INCLUDED
#define OBSTACLEDETECTION_H_INCLUDED

#define DISTANCE_VALUE_COUNT 512

/**
 * Defines the steps array size for obstacle detection which is the maximum number
 * of objects that can be detected with the current algorithm.
 */
#define MAXIMUM_DETECTABLE_OBJECTS 128


#define SCANNER_MIN_DISTANCE 20
#define SCANNER_MAX_DISTANCE 5600

// 360 divided by 1024 degrees
#define RESOLUTION 0.3515625f

// threshold to decide new or same segment is dynamically calculated by this factor
#define THRESHOLD_FACTOR 0.033f // 16.5mm divided by 500mm

typedef struct ransac_triangle {
    int x1;
    int y1;
    int x2;
    int y2;
    int x3;
    int y3;
} ransac_triangle;

/**
 * Struct that encapsulates all data necessary in the obstacle detection process.
 */
typedef struct obstacle_detection_data {
    int timestamp;

    int nearest_steps[MAXIMUM_DETECTABLE_OBJECTS]; // index = Obstacle-ID, integer-value = Nearest-Step of Obstacle
    int first_steps[MAXIMUM_DETECTABLE_OBJECTS]; // index = Obstacle-ID, integer-value = First-Step of Obstacle
    int last_steps[MAXIMUM_DETECTABLE_OBJECTS]; // index = Obstacle-ID, integer-value = Last-Step of Obstacle

    int distances[DISTANCE_VALUE_COUNT]; // 18 bit decoded distance-values in millimeter for each measurement step

    int obid; // Last object ID detected. After complete obstacle detection, this is the
              // number of detected obstacles counting from zero!

    // Stores the results of the RANSAC model computation per object id
    ransac_triangle ransac_results[MAXIMUM_DETECTABLE_OBJECTS];
} obstacle_detection_data;




/**
 * Creates and initializes an obstacle_detection_data struct and returns
 * a pointer to it. This pointer must be manually freed.
 */
obstacle_detection_data* obstacle_detection_init_memory();

void obstacle_detection_zero_memory(obstacle_detection_data* data);

int evaluate_scanner_segment(obstacle_detection_data* data, char *segment);
void detect_obstacle_segments(obstacle_detection_data* data);
void do_ransac(obstacle_detection_data* data);
void visualize(obstacle_detection_data* data);



#endif
