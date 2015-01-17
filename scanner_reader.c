#include <stdio.h> /* for fprintf and stderr */
#include <stdlib.h> /* for exit */
#include <string.h>
#include <stdbool.h>
#include <fcntl.h> /* open syscall */
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <assert.h>

#include "scanner_reader.h"

int scanner_data_offset = 0;
int scanner_data_length = -1;
char *scanner_data;

// Counting processed frames for debugging purpose
int segment_count = 0;

int fd = -1;
FILE *fp = NULL;
FILE *log_fp = NULL;
bool scanner_attached = false;

static bool scanner_initialized = false;

FILE *scanner_open(const char *path, bool log_scanner_output) {
    int status;
    struct stat fd_stat;

    fd = open(path, O_RDWR);
    if(fd == -1) {
        fprintf(stderr, "Could not open file %s (%s:%d).\n", path, __FILE__, __LINE__);
        perror("Error");
        exit(1);
    }

    fp = fdopen(fd, "r+b");
    if(fp == NULL) {
        fprintf(stderr, "Could not open file %s (%s:%d).\n", path, __FILE__, __LINE__);
        perror("Error");
        exit(1);
    }

    status = fstat(fd, &fd_stat);
    if(status == -1) {
        fprintf(stderr, "Unable to stat file %s (%s:%d).\n", path, __FILE__, __LINE__);
        exit(1);
    }

    if(!S_ISREG(fd_stat.st_mode)) {
        printf("Laser scanner seems to be attached");
        scanner_attached = true;
    } else {
        printf("Laser scanner does NOT seem to be attached");
    }
    printf(" (S_ISCHR: %d, S_ISREG: %d).\n", S_ISCHR(fd_stat.st_mode), S_ISREG(fd_stat.st_mode));

    if(scanner_attached && log_scanner_output) {
        log_fp = fopen("scanner.out", "w");
        if(fp == NULL) {
            fprintf(stderr, "Could not open file 'scanner.out' for scanner logging. (%s:%d).\n", __FILE__, __LINE__);
            exit(1);
        }
    }

    scanner_initialize(fp);
    return fp;
}

/*
 * Reads from the given file descriptor and discards the output until no more data is
 * available. The consumed bytes are returned. If not timeout is given, a default of 1 second
 * will be used.
 */
int fd_flush(int fd, struct timeval *timeout) {
    char buffer[1];
    fd_set set_read;
    FD_ZERO(&set_read);
    FD_SET(fd, &set_read);

    int bytes_read = 0;
    while(1) {
        int s = select(fd+1, &set_read, NULL, NULL, timeout);
        if(s != 1) {
            break;
        }
        int ret = read(fd, buffer, 1);
        assert(ret != -1);

        bytes_read++;
    }

    return bytes_read;
}

void scanner_initialize(FILE *fp) {
    scanner_initialized = true;

    // If we're reading from a file we don't need to send commands to the scanner.
    if(!scanner_attached) {
        return;
    }

    int fd = fileno(fp);
    printf("Resetting laser scanner: ");

    char *cmd = "RS\n";
    int ret = write(fd, cmd, 3);
    assert(ret != -1);

    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    fd_flush(fd, &timeout);

    printf("done\n");

    // M=continuous, D=3byte-encoding, startangle=-90, endangle=+90,
    // clustercount=1, skipinterval=0 --> 640-128 = 512 steps
    printf("Initializing laser scanner: ");

    // TODO: Allow setting the initialization angle
    cmd = "MD0128064001000\n";
    ret = write(fd, cmd, 16);
    assert(ret != -1);

    printf("done\n");
}

void scanner_read(void *ptr, size_t bytes, FILE *fp) {
    size_t objects_read = fread(ptr, bytes, 1, fp);

    // Log scanner output to file if requested
    if(log_fp != NULL) {
        fwrite(ptr, bytes, 1, log_fp);
    }

    if(objects_read != 1) {
        if(fseek(fp, 0L, SEEK_SET) == 0) {
            return scanner_read(ptr, bytes, fp);
        } else {
#ifdef ZYNQ
            fprintf(stderr, "'Couldn't read %d bytes from file' near line %d.\n", bytes, __LINE__);
#else
            fprintf(stderr, "'Couldn't read %ld bytes from file' near line %d.\n", bytes, __LINE__);
#endif
            exit(1);
        }
    }
}

/**
 * Reads an SICP2.0 segment and writes it to the given buffer, who's size should
 * at least be equal to SCANNER_SEGMENT_SIZE.
 * @return bytes read
 */
int read_scanner_segment(char *target_buffer, FILE *fp) {
    segment_count++;

    char header[SCANNER_HEADER_SIZE];
    char data[SCANNER_DATA_SIZE];

    scanner_read(header, SCANNER_HEADER_SIZE, fp);

    // seek next frame
    while(header[0] != 'M' || header[1] != 'D') {
        scanner_read(header, SCANNER_HEADER_SIZE, fp);
    }

    if(header[0] != 'M' || header[1] != 'D') {
        printf("first two bytes of segment: '%c%c'\n", header[0], header[1]);
        fprintf(stderr, "Reading scanner segment %d failed. Unexpected input.\n", segment_count);
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
        fprintf(stderr, "Malformed SCIP2.0 header in segment %d. Did not have line-feed at offset 20.\n", segment_count);
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
        fprintf(stderr, "Malformed SCIP2.0 data packet (segment %d). End of data was not found at %d bytes.\n", segment_count, SCANNER_SEGMENT_SIZE);
        exit(1);
    }

    strncpy(target_buffer, data, SCANNER_DATA_SIZE - 3);

    return SCANNER_SEGMENT_SIZE;
}
