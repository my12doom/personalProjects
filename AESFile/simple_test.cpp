#include <tchar.h>
#include <stdio.h>
#include <Windows.h>
#include <WinSock.h>
#include <time.h>
#include <InitGuid.h>

#include "rijndael.h"
#include "E3DReader.h"
#include "E3DWriter.h"
#include "mysql.h"
#include "..\libchecksum\libchecksum.h"

#pragma  comment(lib, "libmySQL.lib")


// {09571A4B-F1FE-4C60-9760-DE6D310C7C31}
DEFINE_GUID(CLSID_CoreAVC, 
			0x09571A4B, 0xF1FE, 0x4C60, 0x97, 0x60, 0xDE, 0x6D, 0x31, 0x0C, 0x7C, 0x31);

// {D00E73D7-06F5-44F9-8BE4-B7DB191E9E7E}
DEFINE_GUID(CLSID_PD10_DECODER, 
			0xD00E73D7, 0x06f5, 0x44F9, 0x8B, 0xE4, 0xB7, 0xDB, 0x19, 0x1E, 0x9E, 0x7E);

// {F07E981B-0EC4-4665-A671-C24955D11A38}
DEFINE_GUID(CLSID_PD10_DEMUXER, 
			0xF07E981B, 0x0EC4, 0x4665, 0xA6, 0x71, 0xC2, 0x49, 0x55, 0xD1, 0x1A, 0x38);

static const GUID CLSID_my12doomSource = { 0x8FD7B1DE, 0x3B84, 0x4817, { 0xA9, 0x6F, 0x4C, 0x94, 0x72, 0x8B, 0x1A, 0xAE } };
static const GUID CLSID_SSIFSource = { 0x916e4c8d, 0xe37f, 0x4fd4, { 0x95, 0xf6, 0xa4, 0x4e, 0x51, 0x46, 0x2e, 0xdf } };

int _tmain(int argc, _TCHAR* argv[])
{
	FILE * f = fopen("F:\\core.dat", "wb");
	fwrite(&CLSID_CoreAVC, 1, sizeof(CLSID_CoreAVC), f);
	fwrite(&CLSID_my12doomSource, 1, sizeof(CLSID_CoreAVC), f);
	fwrite(&CLSID_SSIFSource, 1, sizeof(CLSID_CoreAVC), f);
	fwrite(&CLSID_PD10_DECODER, 1, sizeof(CLSID_CoreAVC), f);
	fwrite(&CLSID_PD10_DEMUXER, 1, sizeof(CLSID_CoreAVC), f);
	fclose(f);

	if (argc<2)
		return 0;

	if (argc == 4 && argv[3][0] == L'd')
	{
		file_reader reader;
		reader.SetFile(CreateFileW (argv[1], GENERIC_READ, FILE_SHARE_DELETE|FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL));

		if (!reader.m_is_encrypted)
		{
			printf("not a E3D encrypted file.\n");
			return 0;
		}

		// try key file
		unsigned char key[36] = "Hello World!";
		wchar_t keyfile[MAX_PATH];
		wcscpy(keyfile, argv[1]);
		wcscat(keyfile, L".key");
		FILE * key_f = _wfopen(keyfile, L"rb");
		if (key_f)
		{
			fread(key, 1, 32, key_f);
			fclose(key_f);
		}
		reader.set_key(key);
		if (reader.m_key_ok)
		{
			printf("key file OK!.\n");
			goto decrypting;
		}
		else
		{
			printf("key file failed, trying get key from database.\n");
		}		

		//database
		MYSQL mysql;
		mysql_init(&mysql);
		if (mysql_real_connect(&mysql, "127.0.0.1", "root", "tester88", "mydb",3306, NULL, 0))
		{
			printf("db connected.\n");

			// hash2string
			char str_key[65] = "";
			char str_hash[41] = "";
			char tmp[3] = "";
			for(int i=0; i<20; i++)
			{
				sprintf(tmp, "%02X", reader.m_hash[i]);
				strcat(str_hash, tmp);
			}

			// search for existing record
			printf("searching for file %s.\n", str_hash);
			char select_command[1024];
			sprintf(select_command, "select * from movies where hash=\'%s\';", str_hash);

			if(mysql_real_query(&mysql, select_command, (UINT)strlen(select_command)))
			{
				printf("db error.\n");
				exit (-1);
			}

			MYSQL_RES *select_result = mysql_use_result(&mysql);

			int record_found = 0;
			MYSQL_ROW row;
			while (row = mysql_fetch_row(select_result))
			{
				printf("found: record %s,  pass = %s.\n", row[0], row[2]);
				printf("using this key.\n");
				strcpy(str_key, row[2]);
				for(int i=0; i<32; i++)
					sscanf(str_key+2*i, "%02X", key+i);
				record_found++;

				printf("testing key %s....", str_key);
				reader.set_key(key);
				printf(reader.m_key_ok ? "OK!\n" : "FAIL.\n");
				if (reader.m_key_ok)
					break;
			}
			mysql_free_result(select_result);

			if (record_found  == 0)
			{
				printf("no record found in dababase, exiting.\n");
			}
		}

		else
		{
			printf("db fail.\n");
		}
		mysql_close(&mysql);

decrypting:
		// decrypting
		if (!reader.m_key_ok)
			return -1;
		printf("start decrypting..\n");
		FILE *output = _wfopen(argv[2], L"wb");
		const int read_size = 655360;
		char *tmp = new char[read_size];
		LARGE_INTEGER size;
		reader.GetFileSizeEx(&size);
		for(__int64 byte_left= size.QuadPart; byte_left>0; byte_left-= read_size)
		{
			DWORD byte_read = 0;
			reader.ReadFile(tmp, (DWORD)min(byte_left, read_size), &byte_read, NULL);
			fwrite(tmp, byte_read, 1, output);

			if (byte_read < read_size)
				break;

			printf("\r%lld / %lld, %.1f%%", size.QuadPart-byte_left, size.QuadPart, (double)(size.QuadPart-byte_left) * 100 / size.QuadPart);
		}

		delete [] tmp;
		printf("\r%lld/ %lld\n", size.QuadPart, size.QuadPart);
		printf("OK.\n");
		fclose(output);
	}
	else if (argc == 3)
	{
		// hash
		unsigned char hash[20];
		video_checksum(argv[1], (DWORD*)hash);

		char str_hash[41] = "";
		char tmp[3] = "";
		for(int i=0; i<20; i++)
		{
			sprintf(tmp, "%02X", hash[i]);
			strcat(str_hash, tmp);
		}

		// key
		unsigned char key[36] = "The World! testing";		// with a int NULL terminator
		char str_key[65] = "";
		srand(time(NULL));
		for(int i=0; i<32; i++)
		{
			key[i] = rand()%256;
			sprintf(tmp, "%02X", key[i]);
			strcat(str_key, tmp);
		}


		// database
		MYSQL mysql;
		mysql_init(&mysql);
		if (mysql_real_connect(&mysql, "127.0.0.1", "root", "tester88", "mydb",3306, NULL, 0))
		{
			printf("db connected.\n");

			// search for existing record
			printf("searching for file %s.\n", str_hash);
			char select_command[1024];
			sprintf(select_command, "select * from movies where hash=\'%s\';", str_hash);

			if(mysql_real_query(&mysql, select_command, (UINT)strlen(select_command)))
			{
				printf("db error.\n");
				exit (-1);
			}
			
			MYSQL_RES *select_result = mysql_use_result(&mysql);

			int record_found = 0;
			MYSQL_ROW row;
			while (row = mysql_fetch_row(select_result))
			{
				printf("found: record %s,  pass = %s.\n", row[0], row[2]);
				printf("using this key.\n");
				strcpy(str_key, row[2]);
				for(int i=0; i<32; i++)
					sscanf(str_key+2*i, "%02X", key+i);
				record_found++;
			}
			mysql_free_result(select_result);

			if (record_found  == 0)
			{
				printf("no record found, generated new pass = %s.\n", str_key);
			}

			// encoding
			printf("encoding file %s with key = %s.\n", str_hash, str_key);
			WCHAR key_file[MAX_PATH];
			wcscpy(key_file, argv[2]);
			wcscat(key_file, L".key");
			FILE * f = _wfopen(key_file, L"wb");
			if (f){fwrite(key, 1, 32, f); fclose(f);}
			encode_file(argv[1], argv[2], key, hash, stdout);

			if (record_found == 0)
			{
				char insert_command[1024];
				sprintf(insert_command, "insert into movies (hash, pass) values (\'%s\', \'%s\');", str_hash, str_key);
				printf("inserting to database....");
				if (mysql_real_query(&mysql, insert_command, (UINT)strlen(insert_command)))
				{
					printf("DB ERROR : %s.\n", mysql_error(&mysql));
				}
				else
				{
					printf("OK.\n");
				}
			}
		}

		else
		{
			printf("db fail.\n");
		}
		mysql_close(&mysql);
	}






	else if (argc == 2)
	{
		// hash
		file_reader reader;
		HANDLE h_file = CreateFileW (argv[1], GENERIC_READ, FILE_SHARE_DELETE|FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
		reader.SetFile(h_file);
		CloseHandle(h_file);

		if (!reader.m_is_encrypted)
		{
			printf("not a E3D encrypted file, exiting.\n");
			return -1;
		}

		char str_hash[41] = "";
		char tmp[3] = "";
		for(int i=0; i<20; i++)
		{
			sprintf(tmp, "%02X", reader.m_hash[i]);
			strcat(str_hash, tmp);
		}

		// key
		unsigned char key[36] = "";		// with a int NULL terminator
		char str_key[65] = "";
		for(int i=0; i<32; i++)
		{
			key[i] = rand()%256;
			sprintf(tmp, "%02X", key[i]);
			strcat(str_key, tmp);
		}

		wchar_t file_key[MAX_PATH];
		wcscpy(file_key, argv[1]);
		wcscat(file_key, L".key");
		FILE * f = _wfopen(file_key, L"rb");
		if (!f)
		{
			printf("Key file not found, exiting.\n");
			return -1;
		}
		fread(key, 1, 32, f);
		fclose(f);
		reader.set_key(key);
		if (!reader.m_key_ok)
		{
			printf("Key file didn't match, exiting.\n");
			return -1;
		}



		// database
		MYSQL mysql;
		mysql_init(&mysql);
		if (mysql_real_connect(&mysql, "127.0.0.1", "root", "tester88", "mydb",3306, NULL, 0))
		{
			printf("db connected.\n");

			// search for existing record
			printf("searching for file %s.\n", str_hash);
			char select_command[1024];
			sprintf(select_command, "select * from movies where hash=\'%s\';", str_hash);

			if(mysql_real_query(&mysql, select_command, (UINT)strlen(select_command)))
			{
				printf("db error.\n");
				return -1;
			}

			MYSQL_RES *select_result = mysql_use_result(&mysql);

			int record_found = 0;
			MYSQL_ROW row;
			while (row = mysql_fetch_row(select_result))
			{
				printf("found: record %s,  pass = %s.\n", row[0], row[2]);
				strcpy(str_key, row[2]);
				for(int i=0; i<32; i++)
					sscanf(str_key+2*i, "%02X", key+i);
				reader.set_key(key);
				printf(reader.m_key_ok ? "key match.\n":"key didn't match.\n");
				record_found++;
			}
			mysql_free_result(select_result);


			if (record_found == 0 || !reader.m_key_ok)
			{
				char insert_command[1024];
				sprintf(insert_command, "insert into movies (hash, pass) values (\'%s\', \'%s\');", str_hash, str_key);
				printf("inserting to database....");
				if (mysql_real_query(&mysql, insert_command, (UINT)strlen(insert_command)))
				{
					printf("DB ERROR : %s.\n", mysql_error(&mysql));
				}
				else
				{
					printf("OK.\n");
				}
			}
		}

		else
		{
			printf("db fail.\n");
		}
		mysql_close(&mysql);
	}
	return 0;
}