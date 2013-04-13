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

#ifndef MPEG4_WRITER_H_

#define MPEG4_WRITER_H_

#include <stdio.h>

#include <media/stagefright/MediaWriter.h>
#include <utils/List.h>
#include <utils/threads.h>

namespace android {
#ifndef ANDROID_DEFAULT_CODE
//added file cache by hai.li @2010-12-25
#undef USE_FILE_CACHE
#define USE_FILE_CACHE 1
//#define PERFORMANCE_PROFILE
#else
#undef USE_FILE_CACHE
#define USE_FILE_CACHE 0
#endif
#if USE_FILE_CACHE
#define DEFAULT_FILE_CACHE_SIZE 64*1024
#endif
class MediaBuffer;
class MediaSource;
class MetaData;

#if USE_FILE_CACHE
class MPEG4FileCacheWriter;
#endif

class MPEG4Writer : public MediaWriter {
public:
    MPEG4Writer(const char *filename);
    MPEG4Writer(int fd);

    virtual status_t addSource(const sp<MediaSource> &source);
    virtual status_t start(MetaData *param = NULL);
    virtual status_t stop();
    virtual status_t pause();
    virtual bool reachedEOS();
    virtual status_t dump(int fd, const Vector<String16>& args);

    void beginBox(const char *fourcc);
    void writeInt8(int8_t x);
    void writeInt16(int16_t x);
    void writeInt32(int32_t x);
    void writeInt64(int64_t x);
    void writeCString(const char *s);
    void writeFourcc(const char *fourcc);
    void write(const void *data, size_t size);
    void endBox();
    uint32_t interleaveDuration() const { return mInterleaveDurationUs; }
    status_t setInterleaveDuration(uint32_t duration);
    int32_t getTimeScale() const { return mTimeScale; }

protected:
    virtual ~MPEG4Writer();

private:
    class Track;

    FILE *mFile;
#if USE_FILE_CACHE
	friend class MPEG4FileCacheWriter;
	MPEG4FileCacheWriter *mCacheWriter;
#endif
    bool mUse4ByteNalLength;
    bool mUse32BitOffset;
    bool mIsFileSizeLimitExplicitlyRequested;
    bool mPaused;
    bool mStarted;
    off_t mOffset;
    off_t mMdatOffset;
#ifndef ANDROID_DEFAULT_CODE
	bool mTryStreamableFile; //added by hai.li @2010-12-25 to make streamable file optional
#endif
    uint8_t *mMoovBoxBuffer;
    off_t mMoovBoxBufferOffset;
    bool  mWriteMoovBoxToMemory;
    off_t mFreeBoxOffset;
    bool mStreamableFile;
    off_t mEstimatedMoovBoxSize;
    uint32_t mInterleaveDurationUs;
    int32_t mTimeScale;
    int64_t mStartTimestampUs;

    Mutex mLock;

    List<Track *> mTracks;

    List<off_t> mBoxes;

    void setStartTimestampUs(int64_t timeUs);
    int64_t getStartTimestampUs();  // Not const
    status_t startTracks(MetaData *params);
    size_t numTracks();
    int64_t estimateMoovBoxSize(int32_t bitRate);

    struct Chunk {
        Track               *mTrack;        // Owner
        int64_t             mTimeStampUs;   // Timestamp of the 1st sample
        List<MediaBuffer *> mSamples;       // Sample data

        // Convenient constructor
        Chunk(Track *track, int64_t timeUs, List<MediaBuffer *> samples)
            : mTrack(track), mTimeStampUs(timeUs), mSamples(samples) {
        }

    };
    struct ChunkInfo {
        Track               *mTrack;        // Owner
        List<Chunk>         mChunks;        // Remaining chunks to be written
    };

    bool            mIsFirstChunk;
    volatile bool   mDone;                  // Writer thread is done?
    pthread_t       mThread;                // Thread id for the writer
    List<ChunkInfo> mChunkInfos;            // Chunk infos
    Condition       mChunkReadyCondition;   // Signal that chunks are available

    // Writer thread handling
    status_t startWriterThread();
    void stopWriterThread();
    static void *ThreadWrapper(void *me);
    void threadFunc();

    // Buffer a single chunk to be written out later.
    void bufferChunk(const Chunk& chunk);

    // Write all buffered chunks from all tracks
    void writeChunks();

    // Write a chunk if there is one
    status_t writeOneChunk();

    // Write the first chunk from the given ChunkInfo.
    void writeFirstChunk(ChunkInfo* info);

    // Adjust other track media clock (presumably wall clock)
    // based on audio track media clock with the drift time.
    int64_t mDriftTimeUs;
    void setDriftTimeUs(int64_t driftTimeUs);
    int64_t getDriftTimeUs();

    // Return whether the nal length is 4 bytes or 2 bytes
    // Only makes sense for H.264/AVC
    bool useNalLengthFour();

    void lock();
    void unlock();

    // Acquire lock before calling these methods
    off_t addSample_l(MediaBuffer *buffer);
    off_t addLengthPrefixedSample_l(MediaBuffer *buffer);

    inline size_t write(const void *ptr, size_t size, size_t nmemb, FILE* stream);
    bool exceedsFileSizeLimit();
    bool use32BitFileOffset() const;
    bool exceedsFileDurationLimit();
    bool isFileStreamable() const;
#ifndef ANDROID_DEFAULT_CODE
	bool isTryFileStreamable() const;//added by hai.li @2010-12-25 to make streamable file optional
#endif
    void trackProgressStatus(const Track* track, int64_t timeUs, status_t err = OK);
    void writeCompositionMatrix(int32_t degrees);

    MPEG4Writer(const MPEG4Writer &);
    MPEG4Writer &operator=(const MPEG4Writer &);
};

#if USE_FILE_CACHE
class MPEG4FileCacheWriter{
public:
	MPEG4FileCacheWriter(FILE *file, size_t cachesize = DEFAULT_FILE_CACHE_SIZE);

	virtual ~MPEG4FileCacheWriter();

	bool isFileOpen();

	size_t write(const void *ptr, size_t size, size_t num);

	int seek(off_t offset, int refpos);

	int close();

	FILE *getFile();

	void setOwner(MPEG4Writer *owner);
private:

	inline void flush();

	void* mpCache;

	size_t mCacheSize;

	size_t mDirtySize;

	FILE *mFile;

	bool mFileOpen;

	bool mWriteDirty;

	MPEG4Writer* mOwner;

#ifdef PERFORMANCE_PROFILE
	int64_t mTotaltime;
	int64_t mMaxtime;
	int64_t mTimesofwrite;
#endif
};
#endif
}  // namespace android

#endif  // MPEG4_WRITER_H_
