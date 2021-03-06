#include <stdio.h>
#include <jansson.h>
#include <math.h>
#include <limits.h>

#include "rpc.h"
#include "obstacle_detection.h"
#include "visualization.h"

// draw laserscanner at this position

void obstacle_detection_initialize_visualization() {
    initializeRPC();
}

void visualize(obstacle_detection_data* data) {
    int obid = 0;
    int i;

    float g = 90; // start angle from +90 till -90 degrees
    float l = 0.0f; // length of line to be drawn on the surface

    char text[256];

    // Drawing instructions for the current frame are inserted here
    json_t *line_frame = json_array();
    json_t *ransac_frame = json_array();
    json_t *text_frame = json_array();

    unsigned int color_black = get_color(0, 0, 0, 255);
    unsigned int color_red = get_color(255, 0, 0, 255);
    unsigned int color_grey = get_color(0, 0, 0, 180);
    unsigned int current_color = color_black;

    for(i = 0; i < DISTANCE_VALUE_COUNT; i++) {
        g = i * RESOLUTION; // drawing angle
        l = (float) data->distances[i];


        // in case this is a nearest-step of an obstacle
        if(data->nearest_steps[obid] == i) {
            // array out-of-bound check

            float x = l * sin(g * M_PI / 180.0);
            float y = l * cos(g * M_PI / 180.0);

            sprintf(text, "ID: %d", obid);
            drawText(text_frame, text, x, y);

            // Draw RANSAC triangles
            if((data->first_steps[obid] + 1 < DISTANCE_VALUE_COUNT) && (data->last_steps[obid] - 1 > 0)) {
                draw_ransac_triangle(data, ransac_frame, obid);
            }

            current_color = color_red;
            obid++; // continue with next obstacle
        } else {
            current_color = color_black;
        }

        if(data->distances[i] == INT_MAX) {
            current_color = color_grey;
        }

        float x = l * sin(g * M_PI / 180.0);
        float y = l * cos(g * M_PI / 180.0);
        drawLine(line_frame, 0, 0, x, y, current_color);
    }

    sprintf(text, "Total objects found: %d", data->obid+1);
    drawText(text_frame, text, 100, 100);

    json_array_extend(line_frame, ransac_frame);
    json_array_extend(line_frame, text_frame);
    sendCommand(line_frame);
    json_decref(line_frame);
    json_decref(ransac_frame);
    json_decref(text_frame);
}

void draw_ransac_triangle(obstacle_detection_data* data, json_t *frame, int obid) {
    unsigned int color_magenta = get_color(255, 0, 255, 255);
    drawLine(frame,
        data->ransac_results[obid].x1,
        data->ransac_results[obid].y1,
        data->ransac_results[obid].x2,
        data->ransac_results[obid].y2,
        color_magenta);

    drawLine(frame,
        data->ransac_results[obid].x2,
        data->ransac_results[obid].y2,
        data->ransac_results[obid].x3,
        data->ransac_results[obid].y3,
        color_magenta);

    drawLine(frame,
        data->ransac_results[obid].x3,
        data->ransac_results[obid].y3,
        data->ransac_results[obid].x1,
        data->ransac_results[obid].y1,
        color_magenta);
}

// Pack the given color into an integer. None of the values may be greater than
// 255. Alpha 255 = fully opaque
unsigned int get_color(unsigned int r, unsigned int g, unsigned int b, unsigned int a) {
    if(r > 255 || g > 255 || b > 255 || a > 255) {
        fprintf(stderr, "Wrong usage of get_color\n");
        exit(1);
    }

    return (r << 24) | (g << 16) | (b << 8) | a;
}
