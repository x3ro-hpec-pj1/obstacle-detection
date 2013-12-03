#ifndef SCANNERREADER_H_INCLUDED
#define SCANNERREADER_H_INCLUDED


#define SCANNER_SEGMENT_SIZE 1616
int scanner_data_offset = 0;
int scanner_data_length = -1;
char *scanner_data;

int load_input_file(const char *file_path);
int read_scanner_segment(const char *target_buffer);


#endif
