#ifndef _TS_H
#define _TS_H

#ifdef __cplusplus
extern "C" {
#endif

// now support file joining by this format:
// [Path][file0 name]:[file1 name]:[...]
// example: "I:\BDMV\STREAM\00003.m2ts:00004.m2ts:00005.m2ts"

void *open_ts(char *file);
void *get_ts_sub_stream(void *main_stream);
void close_ts(void *stream);
void switch_to_sub_stream(void *stream);
int read_ts_stream(void *stream, void *buf, int size);
int parse_combined_filename(const char *combined, int n, char *out);

#ifdef __cplusplus
}
#endif

#endif