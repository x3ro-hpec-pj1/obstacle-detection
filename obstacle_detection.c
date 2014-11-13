#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <unistd.h>

#include "obstacle_detection.h"

obstacle_detection_data* obstacle_detection_init_memory() {
    obstacle_detection_data* data = malloc(sizeof(obstacle_detection_data));

    data->timestamp = -1;
    data->obid = 0;
    for(int i = 0; i < DISTANCE_VALUE_COUNT; i++) {
        data->distances[i] = 0;
    }

    for(int i = 0; i < MAXIMUM_DETECTABLE_OBJECTS; i++) {
        data->nearest_steps[i] = 0;
        data->first_steps[i] = 0;
        data->last_steps[i] = 0;
    }

    return data;
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
int evaluate_scanner_segment(obstacle_detection_data* data, char *segment) {
    data->timestamp = segment[0] - 0x30;
    data->timestamp <<= 6;
    data->timestamp &= 0xFFFFC0; // clear six-most MSB bits according SCIP 2.0
    data->timestamp |= segment[1] - 0x30;
    data->timestamp <<= 6;
    data->timestamp &= 0xFFFFC0;
    data->timestamp |= segment[2] - 0x30;
    data->timestamp <<= 6;
    data->timestamp &= 0xFFFFC0;
    data->timestamp |= segment[3] - 0x30;

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
        data->distances[idx/3] = 0;
        data->distances[idx/3] = segment[idx] - 0x30;
        data->distances[idx/3] <<= 6;
        data->distances[idx/3] &= 0xFFFFC0; // clear six-most MSB bits
        data->distances[idx/3] |= segment[idx+1] - 0x30;
        data->distances[idx/3] <<= 6;
        data->distances[idx/3] &= 0xFFFFC0;
        data->distances[idx/3] |= segment[idx+2] - 0x30;
    }

     return data->timestamp;
}

/**
 * Perform obstacle detection on the scanner's distance values.
 * @return Number of obstacles that were detected.
 */
void detect_obstacle_segments(obstacle_detection_data* data) {
    int i;

    // Initialize loop variables
    data->first_steps[0] = 0;
    data->nearest_steps[0] = 0;
    data->last_steps[0] = 1;

    // Skip first and last value, otherwise "i-1" would run out of bounds
    for(i = 1; i < (DISTANCE_VALUE_COUNT-1); i++) {
        // According to the laserscanner data-sheet, no accuraccy is guaranteed
        // below 20mm and above 5600mm, so we skip the value.
        if(data->distances[i] < 20 || data->distances[i] > 5600) {
            data->distances[i] = INT_MAX;
            continue;
        }

        // current step is part of same segment
        if(abs(data->distances[i] - data->distances[i-1]) <= (THRESHOLD_FACTOR * data->distances[i] + 20)) {
            // update nearest-step of this segment
            if(data->distances[i] < data->distances[data->nearest_steps[data->obid]]) {
                data->nearest_steps[data->obid] = i;
            }
            data->last_steps[data->obid] = i; // update last step of segment


        // new segment only if at least 4 steps away from previous
        } else if(i - data->first_steps[data->obid] >= 4) {
            data->obid++; // use the next obstacle-ID in the next loop
            data->first_steps[data->obid] = i;
            data->nearest_steps[data->obid] = i;
            data->last_steps[data->obid] = i+1;
        }
    }
}
