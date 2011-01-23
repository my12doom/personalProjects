#pragma once
#include <windows.h>

void isse_yuy2_to_yv12(const BYTE* src, int src_rowsize, int src_pitch, 
                    BYTE* dstY, BYTE* dstU, BYTE* dstV, int dst_pitchY, int dst_pitchUV,
                    int height);
void isse_yuy2_to_yv12_r(const BYTE* src, int src_rowsize, int src_pitch, 
                    BYTE* dstY, BYTE* dstU, BYTE* dstV, int dst_pitchY, int dst_pitchUV,
                    int height);	// this version take only first chroma byte of each pair of YUY2 chroma(for pd10 reverse converting)
bool my_1088_to_YV12(const BYTE* src, int src_rowsize, int src_pitch, BYTE* dstY, BYTE* dstU, BYTE* dstV, int dst_pitchY, int dst_pitchUV, int height = 1080);
bool my_1088_to_YV12_TV(const BYTE* src, int src_rowsize, int src_pitch, BYTE* dstY, BYTE* dstU, BYTE* dstV, int dst_pitchY, int dst_pitchUV, int height = 1080);
bool my_1088_to_YV12_C(const BYTE* src, int src_rowsize, int src_pitch, BYTE* dstY, BYTE* dstU, BYTE* dstV, int dst_pitchY, int dst_pitchUV, int height = 1080);
