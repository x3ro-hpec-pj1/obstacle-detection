#ifndef SCANNERREADER_H_INCLUDED
#define SCANNERREADER_H_INCLUDED


#define SCANNER_SEGMENT_SIZE 1616
#define SCANNER_HEADER_SIZE 20
#define SCANNER_DATA_SIZE (SCANNER_SEGMENT_SIZE - SCANNER_HEADER_SIZE)

FILE *scanner_open(const char *path, bool log_scanner_output);
void scanner_initialize(FILE *fp);
void read_scanner_header(FILE *fp);
int read_scanner_body(char *target_buffer, FILE *fp, char begin_of_data);
int read_scanner_segment(char *target_buffer, FILE *fp);

#endif
