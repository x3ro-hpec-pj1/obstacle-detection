#include <stdio.h>
#include <jansson.h>
#include <math.h>

#include "rpc.h"
#include "obstacle_detection.h"
#include "visualization.h"

// draw laserscanner at this position
const float X_CENTER = 1001.0f;
const float Y_CENTER = 308.0f;

void obstacle_detection_initialize_visualization() {
    initializeRPC();
}

void visualize(obstacle_detection_data* data) {
    int obid = 0;
    int i;

    float g = 90; // start angle from +90 till -90 degrees
    float l = 0.0f; // length of line to be drawn on the surface

    // Drawing instructions for the current frame are inserted here
    json_t *frame = json_array();

    for(i = 0; i < DISTANCE_VALUE_COUNT; i++) {
        g = i * RESOLUTION; // drawing angle
        l = (float) (data->distances[i] >> 2); // length of line

        // in case this is a nearest-step of an obstacle
        if(data->nearest_steps[obid] == i) {
            // array out-of-bound check

            float x = X_CENTER - l * sin(g * M_PI / 180.0);
            float y = Y_CENTER - l * cos(g * M_PI / 180.0);

            char text[256];
            sprintf(text, "ID: %d", obid);
            drawText(frame, text, x, y);

            // Draw RANSAC triangles
            if((data->first_steps[obid] + 1 < DISTANCE_VALUE_COUNT) && (data->last_steps[obid] - 1 > 0)) {
                drawLine(frame,
                    X_CENTER - data->ransac_results[obid].x1,
                    X_CENTER - data->ransac_results[obid].y1,
                    X_CENTER - data->ransac_results[obid].x2,
                    X_CENTER - data->ransac_results[obid].y2);

                drawLine(frame,
                    X_CENTER - data->ransac_results[obid].x2,
                    X_CENTER - data->ransac_results[obid].y2,
                    X_CENTER - data->ransac_results[obid].x3,
                    X_CENTER - data->ransac_results[obid].y3);

                drawLine(frame,
                    X_CENTER - data->ransac_results[obid].x3,
                    X_CENTER - data->ransac_results[obid].y3,
                    X_CENTER - data->ransac_results[obid].x1,
                    X_CENTER - data->ransac_results[obid].y1);
            }

            obid++; // continue with next obstacle
        }

        float x = X_CENTER - l * sin(g * M_PI / 180.0);
        float y = Y_CENTER - l * cos(g * M_PI / 180.0);
        drawLine(frame, X_CENTER, Y_CENTER, x, y);
    }

    sendCommand(frame);
    json_decref(frame);
}
