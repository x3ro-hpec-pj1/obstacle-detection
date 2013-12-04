#include <stdlib.h>
#include <limits.h>

#include "obstacle_detection.h"

// threshold to decide new or same segment is dynamically calculated by this factor
const float THRESHOLD_FACTOR = 0.033f; // 16.5mm divided by 500mm

int nearest_steps[128]; // index = Obstacle-ID, integer-value = Nearest-Step of Obstacle
int first_steps[128]; // index = Obstacle-ID, integer-value = First-Step of Obstacle
int last_steps[128]; // index = Obstacle-ID, integer-value = Last-Step of Obstacle


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
int evaluate_scanner_segment(char *segment, int *distances) {
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
int detect_obstacle_segments(int *distances) {
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
