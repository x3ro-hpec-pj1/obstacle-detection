#include <stdio.h> /* for fprintf and stderr */
#include <stdlib.h> /* for exit */
#include <string.h>

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

/**
 * Reads an SICP2.0 segment and writes it to the given buffer, who's size should
 * at least be equal to SCANNER_SEGMENT_SIZE.
 * @return bytes read
 */
int read_scanner_segment(char *target_buffer) {
    scanner_data_offset = scanner_data_offset % scanner_data_length;

    char *source = scanner_data + scanner_data_offset;
    if(source[0] != 'M' || source[1] != 'D') {
        printf("first two bytes of segment: '%c%c'\n", source[0], source[1]);
        fprintf(stderr, "Reading scanner segment failed. Unexpected input.");
        exit(1);
    }

    int i = 2;
    // Skip command echo
    while(source[i] != '\n' && i < scanner_data_length) { i++; }
    i++; // Skip LF

    char status[3];
    status[0] = source[i];
    status[1] = source[i+1];
    status[2] = '\0';

    int status_code = atoi(status);
    if(status_code != 0 && status_code != 99 ) {
        printf("Status code %d\n", status_code);
        fprintf(stderr, "Status code must be 00 or 99!");
        exit(1);
    }

    i += 2; // Skip status code...
    i += 1; // ... sum
    i += 1; // ... and LF

    // Early end of segment, indicated by two LF
    if(source[i] == '\n') {
        i++;
        scanner_data_offset += i;
        return 0;
    }

    int data_start = i;

    // Seek end of data, which is indicated by "SUM (1-byte),LF,LF"
    while(!(source[i+2] == '\n' && source[i+3] == '\n') && (i+3) < scanner_data_length) {
        i++;
    }
    i++; // Include the last char

    int data_length = i - data_start;
    strncpy(target_buffer, source + data_start, data_length);
    scanner_data_offset += i + 3; // 3 = Sum + LF + LF

    return data_length;
}
