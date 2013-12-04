#ifndef OBSTACLEDETECTION_H_INCLUDED
#define OBSTACLEDETECTION_H_INCLUDED

#define DISTANCE_VALUE_COUNT 512

int evaluate_scanner_segment(char *segment);
int detect_obstacle_segments();
void obstacle_detection_init_memory();

#endif
