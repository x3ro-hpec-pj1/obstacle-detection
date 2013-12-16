#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "obstacle_detection.h"

// threshold to decide new or same segment is dynamically calculated by this factor
const float THRESHOLD_FACTOR = 0.033f; // 16.5mm divided by 500mm

// draw laserscanner at this position
const float X_CENTER = 1001.0f;
const float Y_CENTER = 308.0f;

int nearest_steps[128]; // index = Obstacle-ID, integer-value = Nearest-Step of Obstacle
int first_steps[128]; // index = Obstacle-ID, integer-value = First-Step of Obstacle
int last_steps[128]; // index = Obstacle-ID, integer-value = Last-Step of Obstacle

int timestamp = 0; // 24 bit timestamp received from laserscanner
int distances[DISTANCE_VALUE_COUNT]; // 18 bit decoded distance-values in millimeter for each measurement step

int sendCount = 0;

unsigned int sckt;
void obstacle_detection_init_memory() {
    timestamp = -1;
    for(int i = 0; i < DISTANCE_VALUE_COUNT; i++) {
        distances[i] = 0; // initialize array of distances
    }

    unsigned int s;
    struct sockaddr_un local, remote;
    unsigned int len;
    int status;

    s = socket(AF_UNIX, SOCK_STREAM, 0);
    if(s == -1) {
        fprintf(stderr, "'Could not create socket' near line %d.\n", __LINE__);
        exit(1);
    }

    local.sun_family = AF_UNIX;
    strcpy(local.sun_path, "/Users/lucas/testsckt");
    unlink(local.sun_path);
    len = strlen(local.sun_path) + sizeof(local.sun_family) + 1;
    status = bind(s, (struct sockaddr *) &local, len);
    if(status == -1) {
        fprintf(stderr, "'Could not bind to socket' near line %d.\n", __LINE__);
        exit(1);
    }

    listen(s, 1);
    if(status == -1) {
        fprintf(stderr, "'Could not listen to socket' near line %d.\n", __LINE__);
        exit(1);
    }

    len = sizeof(struct sockaddr_un);
    sckt = accept(s, (struct sockaddr *) &remote, &len);
    if(sckt == -1) {
        fprintf(stderr, "'Failed accepting socket connection' near line %d.\n", __LINE__);
        exit(1);
    }
}

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
int evaluate_scanner_segment(char *segment) {
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
    for(idx = 0; idx < 1536; idx += 3) {
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

/**
 * Perform obstacle detection on the scanner's distance values.
 * @return Number of obstacles that were detected.
 */
int detect_obstacle_segments() {
    int i;

    // Initialize loop variables
    int obid = 0;
    first_steps[0] = 0;
    nearest_steps[0] = 0;
    last_steps[0] = 1;

    // Skip first and last value, otherwise "i-1" would run out of bounds
    for(i = 1; i < (DISTANCE_VALUE_COUNT-1); i++) {
        // According to the laserscanner data-sheet, no accuraccy is guaranteed
        // below 20mm and above 5600mm, so we skip the value.
        if(distances[i] < 20 || distances[i] > 5600) {
            distances[i] = INT_MAX;
            continue;
        }

        // current step is part of same segment
        if(abs(distances[i] - distances[i-1]) <= (THRESHOLD_FACTOR * distances[i] + 20)) {
            // update nearest-step of this segment
            if(distances[i] < distances[nearest_steps[obid]]) {
                nearest_steps[obid] = i;
            }
            last_steps[obid] = i; // update last step of segment


        // new segment only if at least 4 steps away from previous
        } else if(i - first_steps[obid] >= 4) {
            obid++; // use the next obstacle-ID in the next loop
            first_steps[obid] = i;
            nearest_steps[obid] = i;
            last_steps[obid] = i+1;
        }
    }

    // Number of obstacles found. +1 since obid counts from 0.
    return obid + 1;
}

void sendDrawCommand(char *cmd) {
    char buffer[261]; // 256 + 5 bytes length indicator
    int length = strlen(cmd);
    sprintf(buffer, "%03d%s", length, cmd);

    int status;

    sendCount++;
    status = send(sckt, buffer, strlen(buffer), 0);
    if(status == -1) {
        fprintf(stderr, "'Failed to send message to socket' near line %d.\n", __LINE__);
        perror("send()");
        exit(1);
    }

}

void visualize() {
    int obid = 0;
    int i;

    float g = 90; // start angle from +90 till -90 degrees
    float l = 0.0f; // length of line to be drawn on the surface

    for(i = 0; i < DISTANCE_VALUE_COUNT; i++) {
        g = i * RESOLUTION; // drawing angle
        l = (float) (distances[i] >> 2); // length of line

        // in case this is a nearest-step of an obstacle
        if(nearest_steps[obid] == i) {
            // array out-of-bound check
            if((first_steps[obid] + 1 < DISTANCE_VALUE_COUNT) && (last_steps[obid] - 1 > 0)) {
                //doRANSAC(obid); // calculate and draw triangle-model
            }

            float x = X_CENTER - l * sin(g * M_PI / 180.0);
            float y = Y_CENTER - l * cos(g * M_PI / 180.0);

            char cmd[256];
            sprintf(cmd, "{\"type\": \"text\", \"text\": \"ID: %d\", \"x\": %f, \"y\": %f}\n", obid, x, y);
            sendDrawCommand(cmd);

            obid++; // continue with next obstacle

        } else { // default distance in black
            //paint.setColor(android.graphics.Color.BLACK);
        }

        float x = X_CENTER - l * sin(g * M_PI / 180.0);
        float y = Y_CENTER - l * cos(g * M_PI / 180.0);

        char cmd[256];
        sprintf(cmd, "{\"type\": \"line\", \"x1\": %f, \"y1\": %f, \"x2\": %f, \"y2\": %f}\n", X_CENTER, Y_CENTER, x, y);
        sendDrawCommand(cmd);
    }

    sendDrawCommand("\n");
}
