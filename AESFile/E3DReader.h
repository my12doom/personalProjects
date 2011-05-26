#pragma once

#include <windows.h>
#include "rijndael.h"

__int64 str2int64(const char *str);
HRESULT save_key(const char *hint, const char *key);
HRESULT load_key(const char *, char *key);

class file_reader
{
public:
	file_reader();
	~file_reader();
	void set_key(unsigned char*key);
	void SetFile(HANDLE file);
	BOOL ReadFile(LPVOID lpBuffer, DWORD nToRead, LPDWORD nRead, LPOVERLAPPED lpOverlap);
	BOOL GetFileSizeEx(PLARGE_INTEGER lpFileSize);
	BOOL SetFilePointerEx(__in LARGE_INTEGER liDistanceToMove, __out_opt PLARGE_INTEGER lpNewFilePointer, __in DWORD dwMoveMethod);

//protected:
	int m_block_size;
	unsigned char *m_block_cache;
	unsigned char m_keyhint[32];
	__int64 m_cache_pos;
	__int64 m_pos;
	__int64 m_file_size;
	__int64 m_start_in_file;
	__int64 m_layout;

	__int64 m_file_pos_cache;
	bool m_is_encrypted;
	bool m_key_ok;
	HANDLE m_file;
	AESCryptor m_codec;
};