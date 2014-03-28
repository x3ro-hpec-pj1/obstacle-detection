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

#include "scanner_reader.h"

int scanner_data_offset = 0;
int scanner_data_length = -1;
char *scanner_data;

int fd = -1;
FILE *fp = NULL;
bool scanner_attached = false;

static bool scanner_initialized = false;

FILE *scanner_open(const char *path) {
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

    scanner_initialize(fp);
    return fp;
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
    write(fd, cmd, 3);

    char buffer[1];

    fd_set set_read;
    FD_ZERO(&set_read);
    FD_SET(fd, &set_read);
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    while(1) {
        int s = select(fd+1, &set_read, NULL, NULL, &timeout);

        if(s != 1) {
            break;
        }
        read(fd, buffer, 1);
    }
    printf("done\n");

    printf("Initializing laser scanner: ");
    cmd = "MD0128064001000\n";
    write(fd, cmd, 16);
    printf("done\n");
}

void scanner_read(void *ptr, size_t bytes, FILE *fp) {
    size_t objects_read = fread(ptr, bytes, 1, fp);
    if(objects_read != 1) {
        if(fseek(fp, 0L, SEEK_SET) == 0) {
            return scanner_read(ptr, bytes, fp);
        } else {
            fprintf(stderr, "'Couldn't read %ld bytes from file' near line %d.\n", bytes, __LINE__);
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
    char header[SCANNER_HEADER_SIZE];
    char data[SCANNER_DATA_SIZE];

    scanner_read(header, SCANNER_HEADER_SIZE, fp);

    // seek next frame
    while(header[0] != 'M' || header[1] != 'D') {
        scanner_read(header, SCANNER_HEADER_SIZE, fp);
    }

    if(header[0] != 'M' || header[1] != 'D') {
        printf("first two bytes of segment: '%c%c'\n", header[0], header[1]);
        fprintf(stderr, "Reading scanner segment failed. Unexpected input.\n");
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
