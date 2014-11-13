#ifndef VISUALIZATION_H_INCLUDED
#define VISUALIZATION_H_INCLUDED

#include <jansson.h>

void visualize(obstacle_detection_data* data);
void obstacle_detection_initialize_visualization();
unsigned int get_color(unsigned int r, unsigned int g, unsigned int b, unsigned int a);
void draw_ransac_triangle(obstacle_detection_data* data, json_t *frame, int obid);

#endif
