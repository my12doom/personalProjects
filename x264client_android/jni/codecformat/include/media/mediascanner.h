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

#ifndef MEDIASCANNER_H
#define MEDIASCANNER_H

#include <utils/Log.h>
#include <utils/threads.h>
#include <utils/List.h>
#include <utils/Errors.h>
#include <pthread.h>

namespace android {

class MediaScannerClient;
class StringArray;

struct MediaScanner {
    MediaScanner();
    virtual ~MediaScanner();

    virtual status_t processFile(
            const char *path, const char *mimeType,
            MediaScannerClient &client) = 0;

    typedef bool (*ExceptionCheck)(void* env);
    virtual status_t processDirectory(
            const char *path, const char *extensions,
            MediaScannerClient &client,
            ExceptionCheck exceptionCheck, void *exceptionEnv);

    void setLocale(const char *locale);

    // extracts album art as a block of data
    virtual char *extractAlbumArt(int fd) = 0;

protected:
    const char *locale() const;

private:
    // current locale (like "ja_JP"), created/destroyed with strdup()/free()
    char *mLocale;

    status_t doProcessDirectory(
            char *path, int pathRemaining, const char *extensions,
            MediaScannerClient &client, ExceptionCheck exceptionCheck,
            void *exceptionEnv);

    MediaScanner(const MediaScanner &);
    MediaScanner &operator=(const MediaScanner &);
};

class MediaScannerClient
{
public:
    MediaScannerClient();
    virtual ~MediaScannerClient();
    void setLocale(const char* locale);
    void beginFile();
    bool addStringTag(const char* name, const char* value);
    void endFile();

    virtual bool scanFile(const char* path, long long lastModified, long long fileSize) = 0;
    virtual bool handleStringTag(const char* name, const char* value) = 0;
    virtual bool setMimeType(const char* mimeType) = 0;
    virtual bool addNoMediaFolder(const char* path) = 0;

protected:
#if 0
    void convertValues(uint32_t encoding);
#endif
    void convertValues(uint32_t encoding, int i);

protected:
    // cached name and value strings, for native encoding support.
    StringArray*    mNames;
    StringArray*    mValues;

    // default encoding based on MediaScanner::mLocale string
    uint32_t        mLocaleEncoding;
};

}; // namespace android

#endif // MEDIASCANNER_H

