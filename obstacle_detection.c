#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <unistd.h>

#include "obstacle_detection.h"

obstacle_detection_data* obstacle_detection_init_memory() {
    obstacle_detection_data* data = malloc(sizeof(obstacle_detection_data));
    obstacle_detection_zero_memory(data);
    return data;
}

void obstacle_detection_zero_memory(obstacle_detection_data* data) {
    memset(data, 0, sizeof(obstacle_detection_data));
    data->timestamp = -1;
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

        // Set all distances values which are outside of the scanner's accuraccy range
        // to a defined value.
        if(data->distances[i] < SCANNER_MIN_DISTANCE || data->distances[i] > SCANNER_MAX_DISTANCE) {
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

void do_ransac(obstacle_detection_data* data) {
    int i;
    int obid = 0;
    int fcSetSum = 0; // First
    int lcSetSum = 0; // Last
    int scSetSum = 0; // Second
    int cSetSum = 0; // Prelast

    // X+Y coordinates of Nearest-, First-, Last-, Second-, Prelast- and Current-segmentpoint
    float nx, ny,   fx, fy,   lx, ly,   sx, sy,   px, py,   cx, cy;

    // Segmentborder-VectorCoordiantes representing the front-borders of a segment
    float tfx, tfy,   tlx, tly,   tsx, tsy,   tpx, tpy;

    double len, ang; // length and angle variable used repeatly for multiple uses


    for(i = 0; i < DISTANCE_VALUE_COUNT; i++) {
        fcSetSum = 0;
        lcSetSum = 0;
        scSetSum = 0;
        cSetSum = 0;

        if(data->nearest_steps[obid] == i) { // in case this is a nearest-step of an obstacle
            if((data->first_steps[obid] + 1 < DISTANCE_VALUE_COUNT) && (data->last_steps[obid] - 1 > 0)) { // array out-of-bound check

                // Nearest Segmentpoint
                ang = data->nearest_steps[obid] * RESOLUTION;
                len = data->distances[data->nearest_steps[obid]] >> 2;
                nx = (float) (len*sin(ang*M_PI/180));
                ny = (float) (len*cos(ang*M_PI/180));

                // First Segmentpoint
                ang = data->first_steps[obid] * RESOLUTION;
                len = data->distances[data->first_steps[obid]] >> 2;
                fx = (float) (len*sin(ang*M_PI/180));
                fy = (float) (len*cos(ang*M_PI/180));
                tfx = nx - fx;
                tfy = ny - fy;

                // Last Segmentpoint
                ang = data->last_steps[obid] * RESOLUTION;
                len = data->distances[data->last_steps[obid]] >> 2;
                lx = (float) (len*sin(ang*M_PI/180));
                ly = (float) (len*cos(ang*M_PI/180));
                tlx = nx - lx;
                tly = ny - ly;

                // Second Segmentpoint
                ang = (data->first_steps[obid]+1) * RESOLUTION;
                len = data->distances[ data->first_steps[obid]+1 ] >> 2;
                sx = (float) (len*sin(ang*M_PI/180));
                sy = (float) (len*cos(ang*M_PI/180));
                tsx = nx - sx;
                tsy = ny - sy;

                // Prelast Segmentpoint
                ang = (data->last_steps[obid]-1) * RESOLUTION;
                len = data->distances[ data->last_steps[obid]-1 ] >> 2;
                px = (float) (len*sin(ang*M_PI/180));
                py = (float) (len*cos(ang*M_PI/180));
                tpx = nx - px;
                tpy = ny - py;

                // Calculate for each first-half segmentpoint the data->distances to FIRST/SECOND front-border
                for(int current = data->first_steps[obid]+1;current < data->nearest_steps[obid];current++) {
                    ang = current * RESOLUTION;
                    len = data->distances[current] >> 2;
                    cx = (float) (len*sin(ang*M_PI/180));
                    cy = (float) (len*cos(ang*M_PI/180));

                    // First Segmentborder-Factor
                    ang = ( cx*tfx + cy*tfy - (tfx*nx) - (tfy*ny) ) / ( tfx*tfx + tfy*tfy );
                    // Distance = SquareRoot[ X² + Y² ] (rounded)
                    len = (cx - ( ang*tfx + nx )) * (cx - ( ang*tfx + nx )); // fx²
                    len += (cy - ( ang*tfy + ny )) * (cy - ( ang*tfy + ny )); // fy²
                    fcSetSum += sqrt(( len )) + 0.5; // used by First ConcensSet

                    // Second Segmentborder-Factor
                    ang = ( cx*tsx + cy*tsy - (tsx*nx) - (tsy*ny) ) / ( tsx*tsx + tsy*tsy );
                    // Distance = SquareRoot[ X² + Y² ] (rounded)
                    len = (cx - ( ang*tsx + nx )) * (cx - ( ang*tsx + nx )); // sx²
                    len += (cy - ( ang*tsy + ny )) * (cy - ( ang*tsy + ny )); // sy²
                    scSetSum += sqrt(( len )) + 0.5; // used by Second ConcensSet
                }

                // Calculate for each last-half segmentpoint the data->distances to LAST/PRELAST front-border
                for(int current = data->nearest_steps[obid]+1;current < data->last_steps[obid];current++) {
                    ang = current * RESOLUTION;
                    len = data->distances[current] >> 2;
                    cx = (float) (len*sin(ang*M_PI/180));
                    cy = (float) (len*cos(ang*M_PI/180));

                    // Last Segmentborder-Factor
                    ang = ( cx*tlx + cy*tly - (tlx*nx) - (tly*ny) ) / ( tlx*tlx + tly*tly );
                    // Distance = SquareRoot[ X² + Y² ] (rounded)
                    len = (cx - ( ang*tfx + nx )) * (cx - ( ang*tlx + nx )); // lx²
                    len += (cy - ( ang*tfy + ny )) * (cy - ( ang*tly + ny )); // ly²
                    lcSetSum += sqrt(( len )) + 0.5; // used by Last ConcensSet

                    // Prelast Segmentborder-Factor
                    ang = ( cx*tpx + cy*tpy - (tpx*nx) - (tpy*ny) ) / ( tpx*tpx + tpy*tpy );
                    // Distance = SquareRoot[ X² + Y² ] (rounded)
                    len = (cx - ( ang*tpx + nx )) * (cx - ( ang*tpx + nx )); // px²
                    len += (cy - ( ang*tpy + ny )) * (cy - ( ang*tpy + ny )); // py²
                    cSetSum += sqrt(( len )) + 0.5; //  used by  Prelast ConcensSet
                }

               // Select first segment-point by best ConsensSet
               if( scSetSum < fcSetSum ) {
                   fx = sx;
                   fy = sy;
               }

               // Select last segment-point by best ConsensSet
               if( cSetSum < lcSetSum ) {
                   lx = px;
                   ly = py;
               }

               data->ransac_results[obid].x1 = fx;
               data->ransac_results[obid].y1 = fy;
               data->ransac_results[obid].x2 = nx;
               data->ransac_results[obid].y2 = ny;
               data->ransac_results[obid].x3 = lx;
               data->ransac_results[obid].y3 = ly;
            }

            obid++; // continue with next obstacle
        }
    }
}
