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

#ifndef ANDROID_IMEDIAPLAYER_H
#define ANDROID_IMEDIAPLAYER_H

#include <utils/RefBase.h>
#include <binder/IInterface.h>
#include <binder/Parcel.h>

namespace android {

class Parcel;
class ISurface;

class IMediaPlayer: public IInterface
{
public:
    DECLARE_META_INTERFACE(MediaPlayer);

    virtual void            disconnect() = 0;

    virtual status_t        setVideoSurface(const sp<ISurface>& surface) = 0;
    virtual status_t        prepareAsync() = 0;
    virtual status_t        start() = 0;
    virtual status_t        stop() = 0;
    virtual status_t        pause() = 0;
    virtual status_t        isPlaying(bool* state) = 0;
    virtual status_t        seekTo(int msec) = 0;
    virtual status_t        getCurrentPosition(int* msec) = 0;
    virtual status_t        getDuration(int* msec) = 0;
    virtual status_t        reset() = 0;
    virtual status_t        setAudioStreamType(int type) = 0;
    virtual status_t        setLooping(int loop) = 0;
    virtual status_t        Set3DMode(int loop) = 0;
    virtual status_t        setVolume(float leftVolume, float rightVolume) = 0;
    virtual status_t        suspend() = 0;
    virtual status_t        resume() = 0;
    virtual status_t        setAuxEffectSendLevel(float level) = 0;
    virtual status_t        attachAuxEffect(int effectId) = 0;

    // Invoke a generic method on the player by using opaque parcels
    // for the request and reply.
    // @param request Parcel that must start with the media player
    // interface token.
    // @param[out] reply Parcel to hold the reply data. Cannot be null.
    // @return OK if the invocation was made successfully.
    virtual status_t        invoke(const Parcel& request, Parcel *reply) = 0;

    // Set a new metadata filter.
    // @param filter A set of allow and drop rules serialized in a Parcel.
    // @return OK if the invocation was made successfully.
    virtual status_t        setMetadataFilter(const Parcel& filter) = 0;

    // Retrieve a set of metadata.
    // @param update_only Include only the metadata that have changed
    //                    since the last invocation of getMetadata.
    //                    The set is built using the unfiltered
    //                    notifications the native player sent to the
    //                    MediaPlayerService during that period of
    //                    time. If false, all the metadatas are considered.
    // @param apply_filter If true, once the metadata set has been built based
    //                     on the value update_only, the current filter is
    //                     applied.
    // @param[out] metadata On exit contains a set (possibly empty) of metadata.
    //                      Valid only if the call returned OK.
    // @return OK if the invocation was made successfully.
    virtual status_t        getMetadata(bool update_only,
                                        bool apply_filter,
                                        Parcel *metadata) = 0;
};

// ----------------------------------------------------------------------------

class BnMediaPlayer: public BnInterface<IMediaPlayer>
{
public:
    virtual status_t    onTransact( uint32_t code,
                                    const Parcel& data,
                                    Parcel* reply,
                                    uint32_t flags = 0);
};

}; // namespace android

#endif // ANDROID_IMEDIAPLAYER_H
