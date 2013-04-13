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
 */

/*
 * Copyright (C) 2010 The Android Open Source Project
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

#ifndef __LIBS_STREAMINGZIPINFLATER_H
#define __LIBS_STREAMINGZIPINFLATER_H

#include <unistd.h>
#include <inttypes.h>
#include <zlib.h>

namespace android {

class StreamingZipInflater {
public:
    static const size_t INPUT_CHUNK_SIZE = 64 * 1024;
    static const size_t OUTPUT_CHUNK_SIZE = 64 * 1024;

    // Flavor that pages in the compressed data from a fd
    StreamingZipInflater(int fd, off_t compDataStart, size_t uncompSize, size_t compSize);

    // Flavor that gets the compressed data from an in-memory buffer
    StreamingZipInflater(class FileMap* dataMap, size_t uncompSize);

    ~StreamingZipInflater();

    // read 'count' bytes of uncompressed data from the current position.  outBuf may
    // be NULL, in which case the data is consumed and discarded.
    ssize_t read(void* outBuf, size_t count);

    // seeking backwards requires uncompressing fom the beginning, so is very
    // expensive.  seeking forwards only requires uncompressing from the current
    // position to the destination.
    off_t seekAbsolute(off_t absoluteInputPosition);

private:
    void initInflateState();
    int readNextChunk();

    // where to find the uncompressed data
    int mFd;
    off_t mInFileStart;         // where the compressed data lives in the file
    class FileMap* mDataMap;

    z_stream mInflateState;
    bool mStreamNeedsInit;

    // output invariants for this asset
    uint8_t* mOutBuf;           // output buf for decompressed bytes
    size_t mOutBufSize;         // allocated size of mOutBuf
    size_t mOutTotalSize;       // total uncompressed size of the blob

    // current output state bookkeeping
    off_t mOutCurPosition;      // current position in total offset
    size_t mOutLastDecoded;     // last decoded byte + 1 in mOutbuf
    size_t mOutDeliverable;     // next undelivered byte of decoded output in mOutBuf

    // input invariants
    uint8_t* mInBuf;
    size_t mInBufSize;          // allocated size of mInBuf;
    size_t mInTotalSize;        // total size of compressed data for this blob

    // input state bookkeeping
    size_t mInNextChunkOffset;  // offset from start of blob at which the next input chunk lies
    // the z_stream contains state about input block consumption
};

}

#endif
