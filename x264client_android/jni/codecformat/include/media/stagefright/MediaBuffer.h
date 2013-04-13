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

#ifndef MEDIA_BUFFER_H_

#define MEDIA_BUFFER_H_

#include <pthread.h>

#include <utils/Errors.h>
#include <utils/RefBase.h>

namespace android {

class MediaBuffer;
class MediaBufferObserver;
class MetaData;

class MediaBufferObserver {
public:
    MediaBufferObserver() {}
    virtual ~MediaBufferObserver() {}

    virtual void signalBufferReturned(MediaBuffer *buffer) = 0;

private:
    MediaBufferObserver(const MediaBufferObserver &);
    MediaBufferObserver &operator=(const MediaBufferObserver &);
};

class MediaBuffer {
public:
    // The underlying data remains the responsibility of the caller!
    MediaBuffer(void *data, size_t size);

    MediaBuffer(size_t size);

    // Decrements the reference count and returns the buffer to its
    // associated MediaBufferGroup if the reference count drops to 0.
    void release();

    // Increments the reference count.
    void add_ref();

    void *data() const;
    size_t size() const;

    size_t range_offset() const;
    size_t range_length() const;

    void set_range(size_t offset, size_t length);

    sp<MetaData> meta_data();

    // Clears meta data and resets the range to the full extent.
    void reset();

    void setObserver(MediaBufferObserver *group);

    // Returns a clone of this MediaBuffer increasing its reference count.
    // The clone references the same data but has its own range and
    // MetaData.
    MediaBuffer *clone();

    int refcount() const;

#if !defined(ANDROID_DEFAULT_CODE)
    void set_frame_header_offset(int i4FrameHeaderOffset);
    int get_frame_header_offset() const;
#endif  //#if !defined(ANDROID_DEFAULT_CODE)	

protected:
    virtual ~MediaBuffer();

private:
    friend class MediaBufferGroup;
    friend class OMXDecoder;

    // For use by OMXDecoder, reference count must be 1, drop reference
    // count to 0 without signalling the observer.
    void claim();

    MediaBufferObserver *mObserver;
    MediaBuffer *mNextBuffer;
    int mRefCount;

    void *mData;
    size_t mSize, mRangeOffset, mRangeLength;

    bool mOwnsData;

    sp<MetaData> mMetaData;

#if !defined(ANDROID_DEFAULT_CODE)
    int mFrameHeaderOffset;
#endif  //#if !defined(ANDROID_DEFAULT_CODE)    

    MediaBuffer *mOriginal;

    void setNextBuffer(MediaBuffer *buffer);
    MediaBuffer *nextBuffer();

    MediaBuffer(const MediaBuffer &);
    MediaBuffer &operator=(const MediaBuffer &);    
};

#ifndef ANDROID_DEFAULT_CODE // Demon Deng
class MediaBufferSimpleObserver: public MediaBufferObserver {
public:
    virtual void signalBufferReturned(MediaBuffer *buffer);
};
#endif // #ifndef ANDROID_DEFAULT_CODE

}  // namespace android

#endif  // MEDIA_BUFFER_H_
