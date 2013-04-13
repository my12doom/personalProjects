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
 * Copyright (C) 2007 The Android Open Source Project
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

#ifndef ANDROID_SF_ISURFACE_H
#define ANDROID_SF_ISURFACE_H

#include <stdint.h>
#include <sys/types.h>

#include <utils/Errors.h>
#include <utils/RefBase.h>

#include <binder/IInterface.h>

#include <ui/PixelFormat.h>

#include <hardware/hardware.h>
#include <hardware/gralloc.h>

namespace android {

typedef int32_t    SurfaceID;

class IMemoryHeap;
class OverlayRef;
class GraphicBuffer;

class ISurface : public IInterface
{
protected:
    enum {
        REGISTER_BUFFERS = IBinder::FIRST_CALL_TRANSACTION,
        UNREGISTER_BUFFERS,
        POST_BUFFER, // one-way transaction
        CREATE_OVERLAY,
        REQUEST_BUFFER,
        SET_BUFFER_COUNT,

        // add by Ryan
        SET_LAYER_TYPE,
        GET_LAYER_TYPE,
	 // add by MC Lin
        POST_BUFFER_POS, // one-way transaction
        POST_BUFFER_SCP, // one-way transaction
    };

public: 
    DECLARE_META_INTERFACE(Surface);

    /*
     * requests a new buffer for the given index. If w, h, or format are
     * null the buffer is created with the parameters assigned to the
     * surface it is bound to. Otherwise the buffer's parameters are
     * set to those specified.
     */
    virtual sp<GraphicBuffer> requestBuffer(int bufferIdx,
            uint32_t w, uint32_t h, uint32_t format, uint32_t usage) = 0;

    /*
     * sets the number of buffers dequeuable for this surface.
     */
    virtual status_t setBufferCount(int bufferCount) = 0;
    
    // ------------------------------------------------------------------------
    // Deprecated...
    // ------------------------------------------------------------------------

    class BufferHeap {
    public:
        enum {
            /* rotate source image */
            ROT_0     = 0,
            ROT_90    = HAL_TRANSFORM_ROT_90,
            ROT_180   = HAL_TRANSFORM_ROT_180,
            ROT_270   = HAL_TRANSFORM_ROT_270,
        };
        BufferHeap();
        
        BufferHeap(uint32_t w, uint32_t h,
                int32_t hor_stride, int32_t ver_stride, 
                PixelFormat format, const sp<IMemoryHeap>& heap);
        
        BufferHeap(uint32_t w, uint32_t h,
                int32_t hor_stride, int32_t ver_stride, 
                PixelFormat format, uint32_t transform, uint32_t flags,
                const sp<IMemoryHeap>& heap);
        
        ~BufferHeap(); 
        
        uint32_t w;
        uint32_t h;
        int32_t hor_stride;
        int32_t ver_stride;
        PixelFormat format;
        uint32_t transform;
        uint32_t flags;
        sp<IMemoryHeap> heap;
    };
    
    virtual status_t registerBuffers(const BufferHeap& buffers) = 0;
    virtual void postBuffer(ssize_t offset) = 0; // one-way
    virtual void unregisterBuffers() = 0;
    
    virtual sp<OverlayRef> createOverlay(
            uint32_t w, uint32_t h, int32_t format, int32_t orientation) = 0;

    // add by Ryan
    virtual status_t setLayerType(uint32_t type) = 0;
    virtual uint32_t getLayerType() = 0;

    // add by MC Lin
    // variable position
    virtual void postBuffer(ssize_t offset, int32_t x, int32_t y) = 0;
    // variable scope
    virtual void postBuffer(ssize_t offset, uint32_t w, uint32_t h, uint32_t w_stride, uint32_t h_stride) = 0;
};

// ----------------------------------------------------------------------------

class BnSurface : public BnInterface<ISurface>
{
public:
    virtual status_t    onTransact( uint32_t code,
                                    const Parcel& data,
                                    Parcel* reply,
                                    uint32_t flags = 0);
};

// ----------------------------------------------------------------------------

}; // namespace android

#endif // ANDROID_SF_ISURFACE_H
