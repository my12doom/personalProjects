#pragma once
// this is only a linear buffer, no seek support

#include <Windows.h>
#include "CCritSec.h"


class CFileBuffer
{
public:
	BYTE* m_the_buffer;
	int m_buffer_size;
	int m_data_size;//=0

	bool no_more_data;
	bool no_more_remove;		// true: insert will direct return 0, no blocking

	CCritSec cs;

	CFileBuffer(int buffer_size);
	~CFileBuffer();
	int insert(int size, const BYTE*buf);
	int remove(int size, BYTE *buf);
	int block_remove(int size, BYTE *buf);
};