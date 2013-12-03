#ifndef SCANNERREADER_H_INCLUDED
#define SCANNERREADER_H_INCLUDED


#define SCANNER_SEGMENT_SIZE 1616

int load_input_file(const char *file_path);
int read_scanner_segment(char *target_buffer);


#endif
