#include "CFileBuffer.h"

CFileBuffer::CFileBuffer(int buffer_size):
m_buffer_size(buffer_size),
m_data_size(0),
no_more_data(false)
{
	m_the_buffer = new BYTE[buffer_size];
}

CFileBuffer::~CFileBuffer()
{
	delete [] m_the_buffer;
}

int CFileBuffer::insert(int size, const BYTE*buf)
{
retry:
	cs.Lock();
	if (size + m_data_size > m_buffer_size)
	{
		cs.Unlock();		// wait for enough space
		Sleep(10);
		goto retry;
	}

	memcpy(m_the_buffer+m_data_size, buf, size);
	m_data_size += size;

	cs.Unlock();
	return 0;
}

int CFileBuffer::remove(int size, BYTE *buf)
{
	cs.Lock();

	if (m_data_size < size)
	{
		cs.Unlock();
		return -1;	// not enough data
	}

	memcpy(buf, m_the_buffer, size);
	memmove(m_the_buffer, m_the_buffer+size, m_data_size-size);
	m_data_size -= size;

	cs.Unlock();

	return 0;
}
