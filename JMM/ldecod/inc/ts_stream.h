#ifndef _TS_H
#define _TS_H

#ifdef __cplusplus
extern "C" {
#endif

void *open_ts(char *file);
void *get_ts_sub_stream(void *main_stream);
void close_ts(void *stream);
void switch_to_sub_stream(void *stream);
int read_ts_stream(void *stream, void *buf, int size);

#ifdef __cplusplus
}
#endif

#endif