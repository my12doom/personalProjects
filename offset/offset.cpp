#include <stdio.h>
#include <conio.h>
#include <locale.h>
#include "../my12doomSource/src/filters/parser/MpegSplitter/IOffsetMetadata.h"

int wmain(int argc, wchar_t *argv[])
{
	setlocale(LC_ALL, "chs");

	if (argc < 3)
	{
		printf("usage:\n");
		printf("offset.exe 123.offset 00001.ssif.offset 00002.ssif.offset\n");
		_getch();
		exit(0);
	}

	my12doom_offset_metadata_header header = {0};
	memcpy(&header.file_header, "offs", 4);
	header.version = 1;

	FILE * o = _wfopen(argv[1], L"wb");
	char *point_data = new char[2048*2048*4];

	for(int i=2; i<argc; i++)
	{
		my12doom_offset_metadata_header header_i = {0};
		FILE * f = _wfopen(argv[i], L"rb");

		if (f == NULL)
		{
			wprintf(L"error opening file %s!\n", argv[i]);
			exit(-1);
		}


		if (fread(&header_i, 1, sizeof(header_i), f) != sizeof(header_i))
		{
			wprintf(L"error reading file %s!\n", argv[i]);
			exit(-1);
		}

		if (header_i.version != 1 || memcmp(&header_i.file_header, "offs", 4) !=0
			|| header_i.fps_numerator == 0 || header_i.fps_denumerator == 0)
		{
			wprintf(L"corrupted file %s!\n", argv[i]);
			exit(-2);
		}

		if (fread(point_data, 1, header_i.point_count, f) != header_i.point_count)
		{
			wprintf(L"corrupted file %s!\n", argv[i]);
			exit(-3);
		}

		header.fps_numerator = header.fps_numerator == 0 ? header_i.fps_numerator : header.fps_numerator;
		header.fps_denumerator = header.fps_denumerator == 0 ? header_i.fps_denumerator : header_i.fps_denumerator;

		header.point_count += header_i.point_count;

		if (header.fps_numerator != header_i.fps_numerator || header.fps_denumerator != header_i.fps_denumerator)
		{
			wprintf(L"FPS missmatch!\n", argv[i]);
			exit(-4);
		}

		fseek(o, 0, SEEK_SET);
		fwrite(&header, 1, sizeof(header), o);
		fseek(o, 0, SEEK_END);
		fwrite(point_data, 1, header_i.point_count, o);
		fflush(o);

		wprintf(L"%d frames in file %s merged.\n", header_i.point_count, argv[i]);
		fclose(f);
	}

	fclose(o);

	return 0;
}