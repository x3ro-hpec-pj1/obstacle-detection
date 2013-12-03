#include <stdio.h> /* for fprintf and stderr */
#include <stdlib.h> /* for exit */
#include <string.h>
#include <stdbool.h>

#include "scanner_reader.h"

int scanner_data_offset = 0;
int scanner_data_length = -1;
char *scanner_data;

int load_input_file(const char *file_path) {
    FILE *fp = fopen(file_path, "rb");
    if(fp == NULL) {
        fprintf(stderr, "'Could not open file at path %s' near line %d.\n", file_path, __LINE__);
        exit(1);
    }

    fseek(fp, 0L, SEEK_END);
    int length = ftell(fp);
    fseek(fp, 0L, SEEK_SET);

    scanner_data = calloc(1, length+1);
    fread(scanner_data, length, 1, fp);

    scanner_data[length] = '\0';
    scanner_data_length = length;

    fclose(fp);
    return 0;
}

void scanner_read(void *ptr, size_t bytes, FILE *fp) {
    size_t objects_read = fread(ptr, bytes, 1, fp);
    //fprintf(stderr, "read: %d, to-read: %d\n", bytes_read, bytes);
    if(objects_read != 1) {
        fprintf(stderr, "'Couldn't read %ld bytes from file' near line %d.\n", bytes, __LINE__);
        exit(1);
    }
}


char *carry_buffer;
int carried_bytes = 0;
bool carry_buffer_allocated = false;

/**
 * Reads an SICP2.0 segment and writes it to the given buffer, who's size should
 * at least be equal to SCANNER_SEGMENT_SIZE.
 * @return bytes read
 */
int read_scanner_segment(char *target_buffer, FILE *fp) {
    //scanner_data_offset = scanner_data_offset % scanner_data_length;
    //int initial_offset = scanner_data_offset;

    // +1, because we need to look at the first byte after the header
    // in order to determine whether the segment ended after it, which is
    // indicated by two consecutive line-feeds.
    char header[SCANNER_HEADER_SIZE];
    char data[SCANNER_DATA_SIZE];

    scanner_read(header, SCANNER_HEADER_SIZE, fp);

    if(header[0] != 'M' || header[1] != 'D') {
        printf("first two bytes of segment: '%c%c'\n", header[0], header[1]);
        fprintf(stderr, "Reading scanner segment failed. Unexpected input.");
        exit(1);
    }

    char status[3];
    status[0] = header[16];
    status[1] = header[17];
    status[2] = '\0';

    int status_code = atoi(status);
    if(status_code != 0 && status_code != 99 ) {
        printf("Status code %d\n", status_code);
        fprintf(stderr, "Status code must be 00 or 99!");
        exit(1);
    }

    // Make sure the header is correct
    if(header[19] != '\n') {
        fprintf(stderr, "Malformed SCIP2.0 header. Did not have line-feed at offset 20.\n");
        exit(1);
    }

    char begin_of_data[1];
    scanner_read(begin_of_data, 1, fp);

    // Early end of packet, which did not contain any data.
    if(begin_of_data[0] == '\n') {
        return SCANNER_HEADER_SIZE + 1; // +1 since we read the additional LF
    }

    data[0] = begin_of_data[0];
    scanner_read(data+1, SCANNER_DATA_SIZE - 1, fp); // -1 since we already read one byte

    if(data[SCANNER_DATA_SIZE - 1] != '\n' || data[SCANNER_DATA_SIZE - 2] != '\n') {
        fprintf(stderr, "Malformed SCIP2.0 data packet. End of data was not found at %d bytes.\n", SCANNER_SEGMENT_SIZE);
        exit(1);
    }

    strncpy(target_buffer, data, SCANNER_DATA_SIZE - 3);

    return SCANNER_SEGMENT_SIZE;
}
