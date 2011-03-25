#include <Windows.h>
#include <stdio.h>
#include <locale.h>
//#include <vld.h>
#include "srt_parser.h"

void main()
{
	setlocale( LC_ALL, "CHS" );

	srt_parser srt;
	srt.init(2000, 200000);
	srt.load(L"Z:\\side_by_side_1080_very_low.LE.srt");

	wchar_t *test = new wchar_t[99999];
	srt.get_subtitle(1000, 4000, test);
	srt.get_subtitle(1000, 4000, test);
	srt.get_subtitle(1000, 4000, test);
	srt.get_subtitle(1000, 4000, test);

	delete [] test;
}