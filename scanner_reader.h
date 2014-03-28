#ifndef SCANNERREADER_H_INCLUDED
#define SCANNERREADER_H_INCLUDED


#define SCANNER_SEGMENT_SIZE 1616
#define SCANNER_HEADER_SIZE 20
#define SCANNER_DATA_SIZE (SCANNER_SEGMENT_SIZE - SCANNER_HEADER_SIZE)

FILE *scanner_open(const char *path);
void scanner_initialize(FILE *fp);
int read_scanner_segment(char *target_buffer, FILE *fp);

#endif
