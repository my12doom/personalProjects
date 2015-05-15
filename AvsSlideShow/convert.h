#pragma once

#include<stdio.h>
#include<stdlib.h>
#include<memory.h>

// Conversion from RGB24 to YUV420
void InitLookupTable();
void InitConvertTable();

//int ConvertRGB2YUV(int w,int h,unsigned char *rgbdata,unsigned int *yuv);
int ConvertRGBA2YUV(int w,int h,unsigned char *bmp,unsigned char*yuv) ;

// Conversion from YUV420 to RGB24
void ConvertYUV2RGB(unsigned char *src0,unsigned char *src1,unsigned char *src2,unsigned char *dst_ori,int width,int height);