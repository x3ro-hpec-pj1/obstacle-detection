#ifndef OBSTACLEDETECTION_H_INCLUDED
#define OBSTACLEDETECTION_H_INCLUDED

#define DISTANCE_VALUE_COUNT 512

// 360 divided by 1024 degrees
#define RESOLUTION 0.3515625f


int evaluate_scanner_segment(char *segment);
int detect_obstacle_segments();
void visualize();

void obstacle_detection_init_memory();

#endif
