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

#ifndef ANDROID_AUDIO_TRACK_SHARED_H
#define ANDROID_AUDIO_TRACK_SHARED_H

#include <stdint.h>
#include <sys/types.h>

#include <utils/threads.h>

namespace android {

// ----------------------------------------------------------------------------

#define THREAD_PRIORITY_AUDIO_CLIENT (ANDROID_PRIORITY_AUDIO)
// Maximum cumulated timeout milliseconds before restarting audioflinger thread
#define MAX_STARTUP_TIMEOUT_MS  3000    // Longer timeout period at startup to cope with A2DP init time
#define MAX_RUN_TIMEOUT_MS      1000
#define WAIT_PERIOD_MS          20      // For Lower Power //10

#define CBLK_UNDERRUN_MSK       0x0001
#define CBLK_UNDERRUN_ON        0x0001  // underrun (out) or overrrun (in) indication
#define CBLK_UNDERRUN_OFF       0x0000  // no underrun
#define CBLK_DIRECTION_MSK      0x0002
#define CBLK_DIRECTION_OUT      0x0002  // this cblk is for an AudioTrack
#define CBLK_DIRECTION_IN       0x0000  // this cblk is for an AudioRecord
#define CBLK_FORCEREADY_MSK     0x0004
#define CBLK_FORCEREADY_ON      0x0004  // track is considered ready immediately by AudioFlinger
#define CBLK_FORCEREADY_OFF     0x0000  // track is ready when buffer full
#define CBLK_INVALID_MSK        0x0008
#define CBLK_INVALID_ON         0x0008  // track buffer is invalidated by AudioFlinger:
#define CBLK_INVALID_OFF        0x0000  // must be re-created
#define CBLK_DISABLED_MSK       0x0010
#define CBLK_DISABLED_ON        0x0010  // track disabled by AudioFlinger due to underrun:
#define CBLK_DISABLED_OFF       0x0000  // must be re-started

struct audio_track_cblk_t
{

    // The data members are grouped so that members accessed frequently and in the same context
    // are in the same line of data cache.
                Mutex       lock;
                Condition   cv;
    volatile    uint32_t    user;
    volatile    uint32_t    server;
                uint32_t    userBase;
                uint32_t    serverBase;
                void*       buffers;
                uint32_t    frameCount;
                // Cache line boundary
                uint32_t    loopStart;
                uint32_t    loopEnd;
                int         loopCount;
    volatile    union {
                    uint16_t    volume[2];
                    uint32_t    volumeLR;
                };
                uint32_t    sampleRate;
                // NOTE: audio_track_cblk_t::frameSize is not equal to AudioTrack::frameSize() for
                // 8 bit PCM data: in this case,  mCblk->frameSize is based on a sample size of
                // 16 bit because data is converted to 16 bit before being stored in buffer

                uint8_t     frameSize;
                uint8_t     channelCount;
                uint16_t    flags;

                uint16_t    bufferTimeoutMs; // Maximum cumulated timeout before restarting audioflinger
                uint16_t    waitTimeMs;      // Cumulated wait time

                uint16_t    sendLevel;
                uint16_t    reserved;
                // Cache line boundary (32 bytes)
                            audio_track_cblk_t();
                uint32_t    stepUser(uint32_t frameCount);
                bool        stepServer(uint32_t frameCount);
                void*       buffer(uint32_t offset) const;
                uint32_t    framesAvailable();
                uint32_t    framesAvailable_l();
                uint32_t    framesReady();
};


// ----------------------------------------------------------------------------

}; // namespace android

#endif // ANDROID_AUDIO_TRACK_SHARED_H
