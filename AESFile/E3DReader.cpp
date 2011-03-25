#include <stdio.h>
#include "E3DReader.h"

void log_line(char *format, ...)
{
	char tmp[10240];
	va_list valist;
	va_start(valist, format);
	wvsprintfA(tmp, format, valist);
	va_end(valist);

	static FILE * f = fopen("F:\\e3d.log", "wb");
	fprintf(f, "%s(%d)\r\n", tmp, GetCurrentThreadId());
	fflush(f);
}


__int64 str2int64(char *str)
{
	__int64 rtn = 0;
	int len = strlen(str);
	for(int i=0; i<min(8,len); i++)
		((char*)&rtn)[i] = str[i];

	return rtn;
}


file_reader::file_reader()
{
	m_cache_pos = -1;
	m_is_encrypted = false;
	m_file = INVALID_HANDLE_VALUE;
	m_block_cache = NULL;
}

file_reader::~file_reader()
{
	if (m_block_cache)
		free(m_block_cache);
}

void file_reader::set_key(unsigned char*key)
{
	if (!m_is_encrypted)
		printf("warning: not a encrypted file, ignored and set key.\n");

	m_codec.set_key(key, 256);
	m_codec.encrypt(m_keyhint, m_keyhint);
	if (memcmp(m_keyhint, m_keyhint+16, 16))
	{
		printf("key error.\n");
	}
	else
	{
		printf("key ok.\n");
	}

	m_codec.decrypt(m_keyhint, m_keyhint);
}

void file_reader::SetFile(HANDLE file)
{
	// init
	LARGE_INTEGER li;
	li.QuadPart = 0;
	::SetFilePointerEx(file, li, &li, SEEK_CUR);
	m_file = file;
	m_is_encrypted = false;
	if (m_block_cache)
		free(m_block_cache);

	// read parameter
	DWORD byte_read = 0;
	__int64 eightcc;
	if (!::ReadFile(m_file, &eightcc, 8, &byte_read, NULL) || byte_read != 8)
		goto rewind;
	if (eightcc != str2int64("my12doom"))
		goto rewind;

	__int64 size;
	if (!::ReadFile(m_file, &size, 8, &byte_read, NULL) || byte_read != 8)
		goto rewind;
	m_start_in_file = size + 16;

	unsigned char *tmp = (unsigned char*)malloc(size);
	if (!::ReadFile(m_file, tmp, size, &byte_read, NULL) || byte_read != size)
		goto rewind;

	// set member
	m_file_pos_cache = m_start_in_file;
	m_is_encrypted = true;
	m_cache_pos = -1;
	m_pos = 0;
	m_file_size = *(__int64*)(tmp+0x10);
	m_block_size = *(__int64*)(tmp+0x28);
	m_block_cache = (unsigned char*)malloc(m_block_size);
	memcpy(m_keyhint, tmp+0x40, 32);
	free(tmp);
	return;

rewind:
	::SetFilePointerEx(file, li, NULL, SEEK_SET);
}

BOOL file_reader::ReadFile(LPVOID lpBuffer, DWORD nToRead, LPDWORD nRead, LPOVERLAPPED lpOverlap)
{
	log_line("ReadFile %d", nToRead);
	if (!m_is_encrypted)
		return ::ReadFile(m_file, lpBuffer, nToRead, nRead, lpOverlap);

	// caculate size
	DWORD got = 0;
	nToRead = min(nToRead, m_file_size - m_pos);

	if (nToRead<=0 || m_pos<0)
		return FALSE;

	unsigned char*tmp = (unsigned char*)lpBuffer;
	LARGE_INTEGER li;
	DWORD byte_read = 0;
	if ( (m_pos % m_block_size))		// head
	{
		// cache miss
		if (m_cache_pos != m_pos / m_block_size * m_block_size) 
		{
			m_cache_pos = m_pos / m_block_size * m_block_size;

			li.QuadPart = m_start_in_file + m_cache_pos;
			if (li.QuadPart != m_file_pos_cache)
			{
				m_file_pos_cache = li.QuadPart;
				::SetFilePointerEx(m_file, li, NULL, SEEK_SET);
			}

			::ReadFile(m_file, m_block_cache, m_block_size, &byte_read, NULL);
			m_file_pos_cache += byte_read;
			for(int i=0; i<m_block_size; i+=16)
				m_codec.decrypt(m_block_cache+i, m_block_cache+i);
		}

		// copy from cache
		int byte_to_get_from_cache = min(m_cache_pos + m_block_size - m_pos, nToRead);
		memcpy(tmp, m_block_cache + m_pos - m_cache_pos, byte_to_get_from_cache);
		nToRead -= byte_to_get_from_cache;
		tmp += byte_to_get_from_cache;
		m_pos += byte_to_get_from_cache;
		got += byte_to_get_from_cache;
	}

	// main body
	if (nToRead > m_block_size)
	{
		li.QuadPart = m_pos + m_start_in_file;
		if (li.QuadPart != m_file_pos_cache)
		{
			m_file_pos_cache = li.QuadPart;
			::SetFilePointerEx(m_file, li, NULL, SEEK_SET);
		}
		::ReadFile(m_file, tmp, nToRead / m_block_size * m_block_size, &byte_read, NULL);
		byte_read = nToRead / m_block_size * m_block_size, &byte_read; // assume always right
		m_file_pos_cache += byte_read;
		for(int i=0; i<nToRead / m_block_size * m_block_size; i+=16)
			m_codec.decrypt(tmp+i, tmp+i);

		nToRead -= byte_read;
		tmp += byte_read;
		m_pos += byte_read;
		got += byte_read;
	}

	// tail
	if (nToRead > 0)
	{
		m_cache_pos = m_pos;
		li.QuadPart = m_pos + m_start_in_file;
		if (li.QuadPart != m_file_pos_cache)
		{
			m_file_pos_cache = li.QuadPart;
			::SetFilePointerEx(m_file, li, NULL, SEEK_SET);
		}
		::ReadFile(m_file, m_block_cache, m_block_size, &byte_read, NULL);
		m_file_pos_cache += byte_read;
		for(int i=0; i<m_block_size; i+=16)
			m_codec.decrypt(m_block_cache+i, m_block_cache+i);
		memcpy(tmp, m_block_cache, nToRead);

		m_pos += nToRead;
		got += nToRead;
	}

	if (nRead)
		*nRead = got;

	return TRUE;
}

BOOL file_reader::GetFileSizeEx(PLARGE_INTEGER lpFileSize)
{
	log_line("GetFileSizeEx");
	if (!m_is_encrypted)
		return ::GetFileSizeEx(m_file, lpFileSize);

	if (!lpFileSize)
		return FALSE;
	lpFileSize->QuadPart = m_file_size;
	return TRUE;
}

BOOL file_reader::SetFilePointerEx(__in LARGE_INTEGER liDistanceToMove, __out_opt PLARGE_INTEGER lpNewFilePointer, __in DWORD dwMoveMethod)
{
	log_line("SetFilePointerEx, %d, %d", liDistanceToMove, dwMoveMethod);
	if (!m_is_encrypted)
		return ::SetFilePointerEx(m_file, liDistanceToMove, lpNewFilePointer, dwMoveMethod);

	__int64 old_pos = m_pos;

	if (dwMoveMethod == SEEK_SET)
		m_pos = liDistanceToMove.QuadPart;
	else if (dwMoveMethod == SEEK_CUR)
	{
		m_pos += liDistanceToMove.QuadPart;
	}
	else if (dwMoveMethod == SEEK_END)
		m_pos = m_file_size;


	if (m_pos <0)
	{
		m_pos = old_pos;
		return FALSE;
	}
	if (lpNewFilePointer)
		lpNewFilePointer->QuadPart = m_pos;

	return TRUE;
}
