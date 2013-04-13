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
 * Copyright (C) 2008 The Android Open Source Project
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


#ifndef MEDIAMETADATARETRIEVER_H
#define MEDIAMETADATARETRIEVER_H

#include <utils/Errors.h>  // for status_t
#include <utils/threads.h>
#include <binder/IMemory.h>
#include <media/IMediaMetadataRetriever.h>

namespace android {

class IMediaPlayerService;
class IMediaMetadataRetriever;

// Keep these in synch with the constants defined in MediaMetadataRetriever.java
// class.
enum {
    METADATA_KEY_CD_TRACK_NUMBER = 0,
    METADATA_KEY_ALBUM           = 1,
    METADATA_KEY_ARTIST          = 2,
    METADATA_KEY_AUTHOR          = 3,
    METADATA_KEY_COMPOSER        = 4,
    METADATA_KEY_DATE            = 5,
    METADATA_KEY_GENRE           = 6,
    METADATA_KEY_TITLE           = 7,
    METADATA_KEY_YEAR            = 8,
    METADATA_KEY_DURATION        = 9,
    METADATA_KEY_NUM_TRACKS      = 10,
    METADATA_KEY_WRITER          = 11,
    METADATA_KEY_MIMETYPE        = 12,
    METADATA_KEY_ALBUMARTIST     = 13,
    METADATA_KEY_DISC_NUMBER     = 14,
    METADATA_KEY_COMPILATION     = 15,
#ifdef MTK_DRM_APP
    METADATA_KEY_IS_DRM          = 16,
    METADATA_KEY_DRM_CONTENT_URI = 17,
    METADATA_KEY_DRM_OFFSET      = 18,
    METADATA_KEY_DRM_DATALEN     = 19,
    METADATA_KEY_DRM_RIGHTS_ISSUER = 20,
    METADATA_KEY_DRM_CONTENT_NAME  = 21,
    METADATA_KEY_DRM_CONTENT_DES   = 22,
    METADATA_KEY_DRM_CONTENT_VENDOR = 23,
    METADATA_KEY_DRM_ICON_URI       = 24,   
    METADATA_KEY_DRM_METHOD     = 25,
    METADATA_KEY_DRM_MIME       = 26,
#endif	
    // Add more here...
};

class MediaMetadataRetriever: public RefBase
{
public:
    MediaMetadataRetriever();
    ~MediaMetadataRetriever();
    void disconnect();
    status_t setDataSource(const char* dataSourceUrl);
    status_t setDataSource(int fd, int64_t offset, int64_t length);
    sp<IMemory> getFrameAtTime(int64_t timeUs, int option);
    sp<IMemory> extractAlbumArt();
    const char* extractMetadata(int keyCode);

private:
    static const sp<IMediaPlayerService>& getService();

    class DeathNotifier: public IBinder::DeathRecipient
    {
    public:
        DeathNotifier() {}
        virtual ~DeathNotifier();
        virtual void binderDied(const wp<IBinder>& who);
    };

    static sp<DeathNotifier>                  sDeathNotifier;
    static Mutex                              sServiceLock;
    static sp<IMediaPlayerService>            sService;

    Mutex                                     mLock;
    sp<IMediaMetadataRetriever>               mRetriever;

};

}; // namespace android

#endif // MEDIAMETADATARETRIEVER_H
