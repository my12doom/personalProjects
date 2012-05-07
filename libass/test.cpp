/*
 * Copyright (C) 2006 Evgeniy Stepanov <eugeni.stepanov@gmail.com>
 * Copyright (C) 2009 Grigori Goronzy <greg@geekmind.org>
 *
 * This file is part of libass.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
extern "C" {
#include <ass/ass.h>
};
#include <locale.h>

#pragma comment(lib, "libass.a")
#pragma comment(lib, "libgcc.a")
#pragma comment(lib, "libmingwex.a")
#pragma comment(lib, "libass.a")
#pragma comment(lib, "libfreetype.a")
#pragma comment(lib, "libfontconfig.a")
#pragma comment(lib, "libexpat.a")
#pragma comment(lib, "winmm.lib")


HRESULT save_bitmap(DWORD *data, const wchar_t *filename, int width, int height) ;


typedef struct image_s {
    int width, height, stride;
    unsigned char *buffer;      // RGB32
} image_t;

ASS_Library *ass_library;
ASS_Renderer *ass_renderer;

static image_t *gen_image(int width, int height)
{
    image_t *img = (image_t*)malloc(sizeof(image_t));
    img->width = width;
    img->height = height;
    img->stride = width * 4;
    img->buffer = (unsigned char *) malloc(height * width * 4);
    memset(img->buffer, 0, img->stride * img->height);
    //for (int i = 0; i < height * width * 3; ++i)
    // img->buffer[i] = (i/3/50) % 100;
    return img;
}

#define _r(c)  ((c)>>24)
#define _g(c)  (((c)>>16)&0xFF)
#define _b(c)  (((c)>>8)&0xFF)
#define _a(c)  ((c)&0xFF)

static void blend_single(image_t * frame, ASS_Image *img)
{
    int x, y;
    unsigned char opacity = 255 - _a(img->color);
    unsigned char r = _r(img->color);
    unsigned char g = _g(img->color);
    unsigned char b = _b(img->color);

    unsigned char *src;
    unsigned char *dst;

    src = img->bitmap;
    dst = frame->buffer + (frame->height-1-img->dst_y) * frame->stride + img->dst_x * 4;
    for (y = 0; y < img->h; ++y) {
        for (x = 0; x < img->w; ++x) {
            unsigned k = ((unsigned) src[x]) * opacity / 255;
            // possible endianness problems
            dst[x * 4] = (k * b + (255 - k) * dst[x * 4]) / 255;
            dst[x * 4 + 1] = (k * g + (255 - k) * dst[x * 4 + 1]) / 255;
            dst[x * 4 + 2] = (k * r + (255 - k) * dst[x * 4 + 2]) / 255;
        }
        src += img->stride;
        dst -= frame->stride;
    }
}

static void blend(image_t * frame, ASS_Image *img)
{
    int cnt = 0;
    while (img) {
        blend_single(frame, img);
        ++cnt;
        img = img->next;
    }
    //printf("%d images blended\n", cnt);
}

#include "charset.h"

int main(int argc, char *argv[])
{

	char k = 0xef;

	bool a = k == 0xef;


    const int frame_w = 1920;
    const int frame_h = 1080;

	setlocale(LC_ALL, "CHS");

    if (argc < 4) {
        printf("usage: %s <image file> <subtitle file> <time>\n", argv[0]);
        exit(1);
    }

    char *imgfile = argv[1];
    char *subfile = argv[2];
    double tm = strtod(argv[3], 0);

	FILE * f = fopen(subfile, "rb");
	fseek(f, 0, SEEK_END);
	int file_size = ftell(f);
	fseek(f, 0, SEEK_SET);
	char *src = (char*)malloc(file_size);
	char *utf8 = (char*)malloc(file_size*3);
	fread(src, 1, file_size, f);
	fclose(f);

	int utf8_size = ConvertToUTF8(src, file_size, utf8, file_size*3);

	ass_library = ass_library_init();
	if (!ass_library) {
		printf("ass_library_init failed!\n");
		exit(1);
		}

	//ass_set_message_cb(ass_library, msg_callback, NULL);

	//ass_set_extract_fonts(ass_library, 0);
	//ass_set_style_overrides(ass_library, NULL);


	ass_renderer = ass_renderer_init(ass_library);
	if (!ass_renderer) {
		printf("ass_renderer_init failed!\n");
		exit(1);
		}


	ass_set_frame_size(ass_renderer, frame_w, frame_h);
	ass_set_font_scale(ass_renderer, 1.0);
	//ass_set_hinting(ass_renderer, ASS_HINTING_NORMAL);
	ass_set_fonts(ass_renderer, "Arial", "Sans", 1, "Z:\\fonts.conf", 1);
	
	ASS_Track *track = ass_read_memory(ass_library, utf8, utf8_size, NULL);

	free(src);
	free(utf8);

    if (!track) {
        printf("track init failed!\n");
        return 1;
    }

    ASS_Image *img = NULL;
	int n = 0;
	int changed = 0;
	image_t *frame = gen_image(frame_w, frame_h);
	int n2 = 0;
	int l = GetTickCount();
	timeBeginPeriod(1);
	for(int i=0; i<int(tm*1000); i+=40)
	{
		img = ass_render_frame(ass_renderer, track, i, &changed);

		if (n==0) l = GetTickCount();
		if (changed && img)
		{
			int l = timeGetTime();
			n++;
			memset(frame->buffer, 63, frame->stride * frame->height);
			blend(frame, img);
			wchar_t pathname[MAX_PATH];
			wsprintfW(pathname, L"Z:\\ass%02d.bmp", n);
			save_bitmap((DWORD*)frame->buffer, pathname, frame_w, frame_h);

			//printf("\rrender cost %dms.\t\t\n", timeGetTime()-l);
		}

		n2 ++;

		if (i%10000 == 0)
		printf("\r%d/%d ms rendered, %d frame output.", i, int(tm*1000), n);
	}

    ass_free_track(track);
    ass_renderer_done(ass_renderer);
    ass_library_done(ass_library);


    free(frame->buffer);
    free(frame);

    return 0;
}



HRESULT save_bitmap(DWORD *data, const wchar_t *filename, int width, int height) 
{
	FILE *pFile = _wfopen(filename, L"wb");
	if(pFile == NULL)
		return E_FAIL;

	BITMAPINFOHEADER BMIH;
	memset(&BMIH, 0, sizeof(BMIH));
	BMIH.biSize = sizeof(BITMAPINFOHEADER);
	BMIH.biBitCount = 32;
	BMIH.biPlanes = 1;
	BMIH.biCompression = BI_RGB;
	BMIH.biWidth = width;
	BMIH.biHeight = height;
	BMIH.biSizeImage = ((((BMIH.biWidth * BMIH.biBitCount) 
		+ 31) & ~31) >> 3) * BMIH.biHeight;

	BITMAPFILEHEADER bmfh;
	int nBitsOffset = sizeof(BITMAPFILEHEADER) + BMIH.biSize; 
	LONG lImageSize = BMIH.biSizeImage;
	LONG lFileSize = nBitsOffset + lImageSize;
	bmfh.bfType = 'B'+('M'<<8);
	bmfh.bfOffBits = nBitsOffset;
	bmfh.bfSize = lFileSize;
	bmfh.bfReserved1 = bmfh.bfReserved2 = 0;
	//Write the bitmap file header

	UINT nWrittenFileHeaderSize = fwrite(&bmfh, 1, 
		sizeof(BITMAPFILEHEADER), pFile);
	//And then the bitmap info header

	UINT nWrittenInfoHeaderSize = fwrite(&BMIH, 
		1, sizeof(BITMAPINFOHEADER), pFile);
	//Finally, write the image data itself 

	//-- the data represents our drawing

	UINT nWrittenDIBDataSize = 
		fwrite(data, 1, lImageSize, pFile);

	fclose(pFile);

	return S_OK;
}