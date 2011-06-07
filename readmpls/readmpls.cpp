#include <tchar.h>
#include <stdio.h>
#include <conio.h>
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
int main(int argc, char* argv[])
{
	if (argc < 2)
		return 0;
	FILE *f = fopen(argv[1], "rb");
	if (!f)
		return -1;
	DWORD file_type = readDWORD(f);
	if (file_type != 'MPLS')
		return -2;

	DWORD file_version = readDWORD(f);
	DWORD playlist_pos = readDWORD(f);
	DWORD playlist_marks_pos = readDWORD(f);
	DWORD ext_data_pos = readDWORD(f);


	// playlist
	fseek(f, playlist_pos, SEEK_SET);
	DWORD playlist_length = readDWORD(f);
	readWORD(f);	//reserved
	char playlist[4096];
	memset(playlist, 0, 4096);
	WORD n_playlist = readWORD(f);
	WORD n_subpath = readWORD(f);
	
	DWORD playlist_item_pos = playlist_pos + 10;
	for(int i=0; i<n_playlist; i++)
	{
		fseek(f, playlist_item_pos, SEEK_SET);
		WORD playlist_item_size = readWORD(f);
		
		fread(playlist+6*i, 1, 5, f);
		DWORD codec = readDWORD(f);
		readWORD(f);
		readBYTE(f);
		DWORD time_in = readDWORD(f);
		DWORD time_out = readDWORD(f);

		printf("item:%s.m2ts, length = %dms\r\n", playlist+6*i, (time_out-time_in)/45);

		playlist_item_pos += playlist_item_size+2;		//size itself included
	}

	// ext data
	BYTE n_sub_playlist_item = 0;
	char sub_playlist[4096];
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
							printf("sub item:%s.m2ts\r\n", sub_playlist+6*k);

							sub_playlist_item += sub_playlist_item_length+2;		//size itself included

						}
					}

					subpath_entry_data += subpath_entry_length;
				}
			}
		}

		free(ext_data);
	}

	if (argc >=3 )
	{
		printf("Creating avs script: %s.....", argv[2]);
		FILE *avs = fopen(argv[2], "wb");
		if (avs)
		{
			fprintf(avs, "LoadPlugin(\"MCAvs\")\r\n");

			char path[MAX_PATH];
			strcpy(path, argv[1]);
			int left = 2;
			for(int i=strlen(path)-1; i>0 && left > 0; i--)
				if (path[i] == '\\')
				{
					left --;
					path[i] = NULL;
				}
			strcat(path, "\\STREAM\\");
			fprintf(avs, "path = \"%s\"\r\n", path);

			if (n_playlist == n_sub_playlist_item)
			{
				for(int i=0; i<n_playlist; i++)
				{
					fprintf(avs, "p%d = MCAvs(path+\"%s.m2ts\", path+\"%s.m2ts\")\r\n", i, playlist+6*i, sub_playlist+6*i);
				}
			}
			else
			{
				for(int i=0; i<n_playlist; i++)
				{
					fprintf(avs, "p%d = DirectShowSource(path+\"%s.m2ts\")\r\n", i, playlist+6*i);
				}

			}

			fprintf(avs, "return p0");
			for(int i=1; i<n_playlist; i++)
				fprintf(avs, "+p%d", i);
			fclose(avs);
			printf("done, press any key to exit.");
		}
	}

	fclose(f);
	getch();
	return 0;
}