#include <windows.h>
#include <stdio.h>
#include "libchecksum.h"

unsigned int dwindow_n[32] = {0x5cc5db57, 0x881651da, 0x7981477c, 0xa65785e0, 0x5fe54c72, 0x5247aeff, 0xd2b635f7, 0x24781e90, 0x3d4e298e, 0x92f1bb41, 0x90494ef2, 0x2f0f6eeb, 0xb151c299, 0x6641acf9, 0x8a0681de, 0x9dd53c21, 0x623631ca, 0x906c24ff, 0x14980fce, 0xbbd5419f, 0x1ef0366c, 0xb759416a, 0x1a214b0f, 0x070c6972, 0x382d1969, 0xb9a02637, 0xeadffc31, 0xcd13093a, 0xfd273caf, 0xd2140f85, 0x2800ba50, 0x877d04f0};

LONGLONG FileSize(const wchar_t *filename);
BOOL my_setfilepointer(HANDLE file, LONGLONG target);

bool verify_signature(const DWORD *checksum, const DWORD *signature) // checksum is 20byte, signature is 128byte, true = match
{
	DWORD c[32];
	DWORD m[32];
	DWORD e[32];
	DWORD m1[32];

	BigNumberSetEqualdw(e, 65537, 32);
	memset(m, 0, 128);
	memcpy(m, checksum, 20);
	memcpy(c, signature, 128);

	RSA(m1, c, e, (DWORD*)dwindow_n, 32);

	for(int i=0; i<32; i++)
		if (m[i] != m1[i])
			return false;

	return true;
}


LONGLONG FileSize(const wchar_t *filename)
{
	HANDLE f = CreateFileW(filename, FILE_READ_ACCESS, 7, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if(INVALID_HANDLE_VALUE == f)
		return -1;

	LARGE_INTEGER filesize;
	GetFileSizeEx(f, &filesize);

	CloseHandle(f);

	return filesize.QuadPart;
}

BOOL my_setfilepointer(HANDLE file, LONGLONG target)
{
	LARGE_INTEGER t;
	t.QuadPart = target;
	return SetFilePointerEx(file, t, NULL, FILE_BEGIN);
}

int video_checksum(wchar_t *file, DWORD *checksum)
{
	LONGLONG filesize = FileSize(file);
	if (filesize<=0)
		return -1;

	const int min_data_size  = 128*1024*40;
	DWORD *data = (DWORD*) malloc(min_data_size+8);
	memset(data, 0, min_data_size+8);

	// copy the file size
	memcpy(data+min_data_size/4, &filesize, 8);

	if (filesize<min_data_size)
	{
		// use whole file except the head 64K
		FILE *f = _wfopen(file, L"rb");
		fseek(f, 65536, SEEK_SET);
		fread(data, 1, (size_t)filesize-65536, f);
		fclose(f);
	}
	else
	{
		// use 40*64K blocks around whole file
		HANDLE f = CreateFileW(file, FILE_READ_ACCESS, 7, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

		DWORD got;

		// read block 0, except the head 64K
		my_setfilepointer(f, 65536);
		ReadFile(f, data+65536, 65536, &got, NULL);

		// read block 1-39
		for(int i=1; i<40; i++)
		{
			my_setfilepointer(f, filesize / 40 * i);
			ReadFile(f, data+131072/4*i, 131072, &got, NULL);
		}

		CloseHandle(f);
	}

	SHA1Hash((unsigned char*)checksum, (unsigned char*)data, min_data_size+8);

	free(data);

	return 0;
}

int find_startcode(wchar_t *file)
{
	FILE *f = _wfopen(file, L"rb");
	if (!f)
		return -1;

	const int check_size = 65536;
	char *buf = (char*) malloc(check_size);
	fread(buf, 1, check_size, f);
	fclose(f);

	int rtn = -1;
	for(int i=0; i<check_size-8; i++)
		if (buf[i+0] == 'm' && buf[i+1] == 'y' && buf[i+2] == '1' && buf[i+3] =='2' &&
			buf[i+4] == 'd' && buf[i+5] == 'o' && buf[i+6] == 'o' && buf[i+7] =='m')
		{
			rtn = i+26;
			break;
		}

	free(buf);

	return rtn;
}

int verify_file(wchar_t *file)
{
	int pos = find_startcode(file);
	if (pos<=0)
		return -1;		// my12doom not found

    DWORD sha1[5];
	if (video_checksum(file, sha1)<0)
		return -1;		// checksum fail

	// read signature
	DWORD signature[32];
	FILE *f = _wfopen(file, L"rb");
	fseek(f, pos, SEEK_SET);
	fread(signature, 1, 128, f);
	fclose(f);

	for(int i=0; i<32; i++)
		if (signature[i] != 0)
			goto signature_check;

	return 0;			// unsigned file


signature_check:
	if (verify_signature(sha1, signature))
		return 2;		// match
	else
		return 1;		// not match
}

bool encrypt_license(dwindow_license license, DWORD *encrypt_out)		// false = fail
{
	DWORD e[32];
	BigNumberSetEqualdw(e, 65537, 32);

	RSA(encrypt_out, license.id, e, (DWORD*)dwindow_n, 32);
	return true;
}
bool decrypt_license(dwindow_license *license_out, DWORD *msg)		// out is 128byte, decrypt license from message recieved from server
{
	memcpy(license_out->signature, msg, 128);

	DWORD e[32];
	BigNumberSetEqualdw(e, 65537, 32);

	RSA(license_out->id, license_out->signature, e, (DWORD*)dwindow_n, 32);

	return is_valid_license(*license_out);
}

bool is_valid_license(dwindow_license license)			// true = valid
{
	DWORD e[32];
	BigNumberSetEqualdw(e, 65537, 32);

	DWORD m[32];

	RSA(m, (DWORD*)license.signature, e, (DWORD*)dwindow_n, 32);

	for(int i=0; i<32; i++)
		if (license.id[i] != m[i])
			return false;

	return true;
}
bool load_license(wchar_t *file, dwindow_license *out)	// false = fail
{
	FILE * f = _wfopen(file, L"rb");
	if (!f)
		return false;

	if (fread(out->signature, 1, 128, f) != 128)
		return false;

	DWORD e[32];
	BigNumberSetEqualdw(e, 65537, 32);

	RSA(out->id, out->signature, e, (DWORD*)dwindow_n, 32);

	// TODO: CRC32 checksum

}
bool save_license(wchar_t *file, dwindow_license in)	// false = fail
{
	// TODO
	return false;
}