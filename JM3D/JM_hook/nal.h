#pragma once
#include "..\CFileBuffer.h"

extern const unsigned int max_nal_size;
extern unsigned char *delimeter_buffer;

int nal_type(unsigned char *data);
int read_a_nal(CFileBuffer *f);
int read_a_delimeter(CFileBuffer *f);