#ifndef OBSTACLEDETECTION_H_INCLUDED
#define OBSTACLEDETECTION_H_INCLUDED

#define DISTANCE_VALUE_COUNT 512

int evaluate_scanner_segment(char *segment, int *distances);
int detect_obstacle_segments(int *distances);

#endif
