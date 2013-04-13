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

#ifndef MEDIA_WRITER_H_

#define MEDIA_WRITER_H_

#include <utils/RefBase.h>
#include <media/IMediaRecorderClient.h>

namespace android {

struct MediaSource;
struct MetaData;

struct MediaWriter : public RefBase {
    MediaWriter()
        : mMaxFileSizeLimitBytes(0),
          mMaxFileDurationLimitUs(0) {
    }

    virtual status_t addSource(const sp<MediaSource> &source) = 0;
    virtual bool reachedEOS() = 0;
    virtual status_t start(MetaData *params = NULL) = 0;
    virtual status_t stop() = 0;
    virtual status_t pause() = 0;

    virtual void setMaxFileSize(int64_t bytes) { mMaxFileSizeLimitBytes = bytes; }
    virtual void setMaxFileDuration(int64_t durationUs) { mMaxFileDurationLimitUs = durationUs; }
    virtual void setListener(const sp<IMediaRecorderClient>& listener) {
        mListener = listener;
    }

    virtual status_t dump(int fd, const Vector<String16>& args) {
        return OK;
    }

protected:
    virtual ~MediaWriter() {}
    int64_t mMaxFileSizeLimitBytes;
    int64_t mMaxFileDurationLimitUs;
    sp<IMediaRecorderClient> mListener;

    void notify(int msg, int ext1, int ext2) {
        if (mListener != NULL) {
            mListener->notify(msg, ext1, ext2);
        }
    }
private:
    MediaWriter(const MediaWriter &);
    MediaWriter &operator=(const MediaWriter &);
};

}  // namespace android

#endif  // MEDIA_WRITER_H_
