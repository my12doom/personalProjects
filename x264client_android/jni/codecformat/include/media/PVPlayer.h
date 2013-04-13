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

#ifndef ANDROID_PVPLAYER_H
#define ANDROID_PVPLAYER_H

#include <utils/Errors.h>
#include <media/MediaPlayerInterface.h>
#include <media/Metadata.h>

#define MAX_OPENCORE_INSTANCES 25

#ifdef MAX_OPENCORE_INSTANCES
#include <cutils/atomic.h>
#endif

class PlayerDriver;

namespace android {

class PVPlayer : public MediaPlayerInterface
{
public:
                        PVPlayer();
    virtual             ~PVPlayer();

    virtual status_t    initCheck();

    virtual status_t    setDataSource(
            const char *url, const KeyedVector<String8, String8> *headers);

    virtual status_t    setDataSource(int fd, int64_t offset, int64_t length);
    virtual status_t    setVideoSurface(const sp<ISurface>& surface);
    virtual status_t    prepare();
    virtual status_t    prepareAsync();
    virtual status_t    start();
    virtual status_t    stop();
    virtual status_t    pause();
    virtual bool        isPlaying();
    virtual status_t    seekTo(int msec);
    virtual status_t    getCurrentPosition(int *msec);
    virtual status_t    getDuration(int *msec);
    virtual status_t    reset();
    virtual status_t    setLooping(int loop);
	virtual status_t    Set3DMode(int loop);
    virtual player_type playerType() { return PV_PLAYER; }
    virtual status_t    invoke(const Parcel& request, Parcel *reply);
    virtual status_t    getMetadata(
        const SortedVector<media::Metadata::Type>& ids,
        Parcel *records);

    // make available to PlayerDriver
    void        sendEvent(int msg, int ext1=0, int ext2=0) { MediaPlayerBase::sendEvent(msg, ext1, ext2); }

    PlayerDriver* getPlayerDriver() { return (mIsDataSourceSet ? mPlayerDriver : NULL);}

// <--- Morris Yang ALPS00221177 add for m.youtube.com
    char* getHttpCookie() { return mHttpCookie; }
// --->

private:
    static void         do_nothing(status_t s, void *cookie, bool cancelled) { }
    static void         run_init(status_t s, void *cookie, bool cancelled);
    static void         run_set_video_surface(status_t s, void *cookie, bool cancelled);
    static void         run_set_audio_output(status_t s, void *cookie, bool cancelled);
    static void         run_prepare(status_t s, void *cookie, bool cancelled);
    static void         check_for_live_streaming(status_t s, void *cookie, bool cancelled);

    PlayerDriver*               mPlayerDriver;
    char *                      mDataSourcePath;
// <--- Morris Yang ALPS00221177 add for m.youtube.com
    char *                      mHttpCookie;
// --->
    bool                        mIsDataSourceSet;
    sp<ISurface>                mSurface;
    int                         mSharedFd;
    status_t                    mInit;
    int                         mDuration;

#ifdef MAX_OPENCORE_INSTANCES
    static volatile int32_t     sNumInstances;
#endif

private:
    static void         do_nothing_debug(status_t s, void *cookie, bool cancelled);
};

}; // namespace android

#endif // ANDROID_PVPLAYER_H
