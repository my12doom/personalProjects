#include "readmpls.h"
#include <stdio.h>
#include <Windows.h>

DWORD readDWORD(FILE*f)
{
	unsigned char dat[4];
	fread(dat, 1, 4, f);
	return dat[0]<<24 | dat[1]<<16 | dat[2]<<8 | dat[3];
}
DWORD readDWORD(void*data, int &offset)
{
	unsigned char*dat = (unsigned char*)data +offset;
	offset += 4;
	return dat[0]<<24 | dat[1]<<16 | dat[2]<<8 | dat[3];
}

WORD readWORD(FILE*f)
{
	unsigned char dat[2];
	fread(dat, 1, 2, f);
	return dat[0]<<8 | dat[1];
}
WORD readWORD(void*data, int &offset)
{
	unsigned char*dat = (unsigned char*)data +offset;
	offset += 2;
	return dat[0]<<8 | dat[1];
}

BYTE readBYTE(FILE*f)
{
	BYTE dat;
	fread(&dat, 1, 1, f);
	return dat;
}
BYTE readBYTE(void*data, int &offset)
{
	return ((BYTE*)data)[offset++];
}

int scan_mpls(const char *file, int *main_playlist_count, char *main_playlist, int *items_length, char *sub_playlist, int *sub_playlist_count)
{
	FILE *f = fopen(file, "rb");
	if (!f)
		return -1;
	DWORD file_type = readDWORD(f);
	if (file_type != 'MPLS')
		return -2;

	*main_playlist_count = *sub_playlist_count = 0;

	DWORD file_version = readDWORD(f);
	DWORD playlist_pos = readDWORD(f);
	DWORD playlist_marks_pos = readDWORD(f);
	DWORD ext_data_pos = readDWORD(f);


	// main playlist
	fseek(f, playlist_pos, SEEK_SET);
	DWORD playlist_length = readDWORD(f);
	readWORD(f);	//reserved
	memset(main_playlist, 0, 4096);
	WORD n_playlist = readWORD(f);
	*main_playlist_count = n_playlist;
	WORD n_subpath = readWORD(f);

	DWORD playlist_item_pos = playlist_pos + 10;
	for(int i=0; i<n_playlist; i++)
	{
		fseek(f, playlist_item_pos, SEEK_SET);
		WORD playlist_item_size = readWORD(f);

		fread(main_playlist+6*i, 1, 5, f);
		DWORD codec = readDWORD(f);
		readWORD(f);
		readBYTE(f);
		DWORD time_in = readDWORD(f);
		DWORD time_out = readDWORD(f);

		//printf("item:%s.m2ts, length = %dms\r\n", main_playlist+6*i, (time_out-time_in)/45);

		items_length[i] = (time_out - time_in)/45;

		playlist_item_pos += playlist_item_size+2;		//size itself included
	}

	// ext data
	BYTE n_sub_playlist_item = 0;
	memset(sub_playlist, 0, 4096);
	fseek(f, ext_data_pos, SEEK_SET);
	DWORD ext_data_length = readDWORD(f);
	if (ext_data_length>0)
	{
		BYTE *ext_data = (BYTE*)malloc(ext_data_length);
		fread(ext_data, 1, ext_data_length, f);

		int offset = 7;
		BYTE n_ext_data_entries = readBYTE(ext_data, offset);
		

		for(int i=0; i<n_ext_data_entries; i++)
		{
			WORD ID1 = readWORD(ext_data, offset);
			WORD ID2 = readWORD(ext_data, offset);
			DWORD start_address = readDWORD(ext_data, offset)-4;	// ext_data_length is included
			DWORD length = readDWORD(ext_data, offset);

			if (ID1 == 2 && ID2 == 2)
			{
				BYTE *subpath_data = ext_data + start_address;

				int offset = 0;
				DWORD subpath_data_length = readDWORD(subpath_data, offset);
				WORD n_entries = readWORD(subpath_data, offset);

				BYTE * subpath_entry_data = subpath_data + offset;
				for (int j=0; j<n_entries; j++)
				{
					int offset = 0;
					DWORD subpath_entry_length = readDWORD(subpath_entry_data, offset);
					readBYTE(subpath_entry_data, offset);
					BYTE subpath_type = readBYTE(subpath_entry_data, offset);

					if (subpath_type == 8)
					{
						n_sub_playlist_item = *(subpath_entry_data + offset + 3);
						BYTE *sub_playlist_item = subpath_entry_data + offset + 4;
						for(int k=0; k<n_sub_playlist_item; k++)
						{
							int offset = 0;
							WORD sub_playlist_item_length = readWORD(sub_playlist_item, offset);

							memcpy(sub_playlist+6*k, sub_playlist_item+offset, 5);
							//printf("sub item:%s.m2ts\r\n", sub_playlist+6*k);
							(*sub_playlist_count)++;
							sub_playlist_item += sub_playlist_item_length+2;		//size itself included

						}
					}

					subpath_entry_data += subpath_entry_length;
				}
			}
		}

		free(ext_data);
	}

	fclose(f);
	return 0;
}