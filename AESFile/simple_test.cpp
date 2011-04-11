#include <tchar.h>
#include <stdio.h>
#include <Windows.h>
#include "rijndael.h"
#include "E3DReader.h"
#include "E3DWriter.h"

int _tmain(int argc, _TCHAR* argv[])
{
	if (argc<3)
		return 0;

	if (argc == 4 && argv[3][0] == L'd')
	{
		file_reader reader;
		reader.SetFile(CreateFileW (argv[1], GENERIC_READ, FILE_SHARE_DELETE|FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL));
		unsigned char key[32] = "Hello World!";
		reader.set_key(key);
		FILE *output = _wfopen(argv[2], L"wb");
		char tmp[65536];
		LARGE_INTEGER size;
		reader.GetFileSizeEx(&size);
		for(__int64 byte_left= size.QuadPart; byte_left>0; byte_left-= 65536)
		{
			DWORD byte_read = 0;
			reader.ReadFile(tmp, (DWORD)min(byte_left, 65536), &byte_read, NULL);
			fwrite(tmp, byte_read, 1, output);

			if (byte_read < 65536)
				break;
		}

		fclose(output);
	}
	else
	{
		encode_file(argv[1], argv[2]);
	}


	/*
	unsigned char key[32] = "Hello World!";
	file_reader f;
	f.SetFile(CreateFileW (L"F:\\00013.e3d", GENERIC_READ, FILE_SHARE_DELETE|FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL));
	f.set_key(key);
	
	file_reader fo;
	fo.SetFile(CreateFileW (L"F:\\00019hsbs.ssif", GENERIC_READ, FILE_SHARE_DELETE|FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL));


	char *data1 = (char*)malloc(20480000);
	char *datao = (char*)malloc(20480000);

	DWORD got = 0;
	LARGE_INTEGER li;
	for(int i=0; i<500; i++)
	{
		int size = (rand() << 8) + rand();
		size = min(size, 2048000);


		memset(data1, 0, 20480000);
		memset(datao, 0, 20480000);

		li.QuadPart = (rand() << 8) + rand();
		li.QuadPart = f.m_file_size;
		fo.SetFilePointerEx(li, NULL, SEEK_SET);
		f.SetFilePointerEx(li, NULL, SEEK_SET);
		fo.ReadFile(datao, size, &got, NULL);
		f.ReadFile(data1, size, &got, NULL);
		printf("%d", memcmp(data1, datao, size));
	}
	*/

	return 0;
}