/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 *
 * MediaTek Inc. (C) 2010. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek Software")
 * have been modified by MediaTek Inc. All revisions are subject to any receiver's
 * applicable license agreements with MediaTek Inc.
 */

/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef COLOR_CONVERTER_H_

#define COLOR_CONVERTER_H_

#include <sys/types.h>

#include <stdint.h>

#include <OMX_Video.h>
#ifndef ANDROID_DEFAULT_CODE
#include <ui/PixelFormat.h>
#endif
namespace android {

#ifndef ANDROID_DEFAULT_CODE
static const int OMX_MTK_COLOR_FormatYUV =  0x7f000001;// OMX_COLOR_FormatVendorMTKYUV;// ;//0x4D544B00;
#endif
struct ColorConverter {
    ColorConverter(OMX_COLOR_FORMATTYPE from, OMX_COLOR_FORMATTYPE to);
    ~ColorConverter();

    bool isValid() const;

    void convert(
            size_t width, size_t height,
            const void *srcBits, size_t srcSkip,
            void *dstBits, size_t dstSkip);
	void convert3d(
            size_t width, size_t height,size_t owidth, size_t oheight,
            const void *srcBits, size_t srcSkip,
            void *dstBits, size_t dstSkip,bool is3d);

private:
	bool mIs3d;
    OMX_COLOR_FORMATTYPE mSrcFormat, mDstFormat;
    uint8_t *mClip;
	
    uint8_t *initClip();

    void convertCbYCrY(
            size_t width, size_t height,
            const void *srcBits, size_t srcSkip,
            void *dstBits, size_t dstSkip);

    void convertYUV420Planar(
            size_t width, size_t height,
            const void *srcBits, size_t srcSkip,
            void *dstBits, size_t dstSkip);

    void convertQCOMYUV420SemiPlanar(
            size_t width, size_t height,
            const void *srcBits, size_t srcSkip,
            void *dstBits, size_t dstSkip);

    void convertYUV420SemiPlanar(
            size_t width, size_t height,
            const void *srcBits, size_t srcSkip,
            void *dstBits, size_t dstSkip);

#ifndef ANDROID_DEFAULT_CODE
    void convertYUV420PlanarToABGR8888(size_t width, size_t height, const void *srcBits, size_t srcSkip, void *dstBits, size_t dstSkip);
    void convertMTKYUVToYUV420(const uint8_t *input_buffer, uint8_t *output_buffer, int height, int width);
    void convertYUVToRGB565HW(const uint8_t *input_buffer, uint8_t *output_buffer, int srcHeight, int srcWidth, int dstHeight, int dstWidth, size_t srcSkip, size_t dstSkip);
    bool HWYUVToRGBConversion(const uint8_t *yuvBuf, uint8_t *rgbBuf, int srcHeight, int srcWidth, int dstHeight, int dstWidth, size_t srcSkip, size_t dstSkip);
    bool SWYUVToRGBConversion(const uint8_t *yuvBuf, uint8_t *rgbBuf, int srcHeight, int srcWidth, int dstHeight, int dstWidth, size_t srcSkip, size_t dstSkip);
    uint32_t mPmemSrcBufVA;
    uint32_t mPmemDstBufPA;
    PixelFormat mPixelFormat;
#endif
    ColorConverter(const ColorConverter &);
    ColorConverter &operator=(const ColorConverter &);
};

}  // namespace android

#endif  // COLOR_CONVERTER_H_
