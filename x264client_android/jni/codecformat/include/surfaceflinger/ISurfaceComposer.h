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
 * Copyright (C) 2006 The Android Open Source Project
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

#ifndef ANDROID_SF_ISURFACE_COMPOSER_H
#define ANDROID_SF_ISURFACE_COMPOSER_H

#include <stdint.h>
#include <sys/types.h>

#include <utils/RefBase.h>
#include <utils/Errors.h>

#include <binder/IInterface.h>

#include <ui/PixelFormat.h>

#include <surfaceflinger/ISurfaceComposerClient.h>

namespace android {
// ----------------------------------------------------------------------------

class ISurfaceComposer : public IInterface
{
public:
    DECLARE_META_INTERFACE(SurfaceComposer);

    enum { // (keep in sync with Surface.java)
        eHidden             = 0x00000004,
        eDestroyBackbuffer  = 0x00000020,
        eSecure             = 0x00000080,
        eNonPremultiplied   = 0x00000100,
        ePushBuffers        = 0x00000200,

        eFXSurfaceNormal    = 0x00000000,
        eFXSurfaceBlur      = 0x00010000,
        eFXSurfaceDim       = 0x00020000,
        eFXSurfaceMask      = 0x000F0000,
    };

    enum {
        ePositionChanged            = 0x00000001,
        eLayerChanged               = 0x00000002,
        eSizeChanged                = 0x00000004,
        eAlphaChanged               = 0x00000008,
        eMatrixChanged              = 0x00000010,
        eTransparentRegionChanged   = 0x00000020,
        eVisibilityChanged          = 0x00000040,
        eFreezeTintChanged          = 0x00000080,
    };

    enum {
        eLayerHidden        = 0x01,
        eLayerFrozen        = 0x02,
        eLayerDither        = 0x04,
        eLayerFilter        = 0x08,
        eLayerBlurFreeze    = 0x10
    };

    enum {
        eOrientationDefault     = 0,
        eOrientation90          = 1,
        eOrientation180         = 2,
        eOrientation270         = 3,
        eOrientationSwapMask    = 0x01
    };
    
    enum {
        eElectronBeamAnimationOn  = 0x01,
        eElectronBeamAnimationOff = 0x10
    };

    // flags for setOrientation
    enum {
        eOrientationAnimationDisable = 0x00000001
    };

    /* create connection with surface flinger, requires
     * ACCESS_SURFACE_FLINGER permission
     */
    virtual sp<ISurfaceComposerClient> createConnection() = 0;

    /* create a client connection with surface flinger
     */
    virtual sp<ISurfaceComposerClient> createClientConnection() = 0;

    /* retrieve the control block */
    virtual sp<IMemoryHeap> getCblk() const = 0;

    /* open/close transactions. requires ACCESS_SURFACE_FLINGER permission */
    virtual void openGlobalTransaction() = 0;
    virtual void closeGlobalTransaction() = 0;

    /* [un]freeze display. requires ACCESS_SURFACE_FLINGER permission */
    virtual status_t freezeDisplay(DisplayID dpy, uint32_t flags) = 0;
    virtual status_t unfreezeDisplay(DisplayID dpy, uint32_t flags) = 0;

    /* Set display orientation. requires ACCESS_SURFACE_FLINGER permission */
    virtual int setOrientation(DisplayID dpy, int orientation, uint32_t flags) = 0;

    /* signal that we're done booting.
     * Requires ACCESS_SURFACE_FLINGER permission
     */
    virtual void bootFinished() = 0;

    /* Capture the specified screen. requires READ_FRAME_BUFFER permission
     * This function will fail if there is a secure window on screen.
     */
    virtual status_t captureScreen(DisplayID dpy,
            sp<IMemoryHeap>* heap,
            uint32_t* width, uint32_t* height, PixelFormat* format,
            uint32_t reqWidth, uint32_t reqHeight) = 0;
    
    virtual status_t captureVideoLayer(
            sp<IMemoryHeap>* heap,
            uint32_t* width,
            uint32_t* height) = 0;
    
    virtual status_t dumpLayer(uint32_t identity) = 0;

    virtual status_t turnElectronBeamOff(int32_t mode) = 0;
    virtual status_t turnElectronBeamOn(int32_t mode) = 0;

    /* Signal surfaceflinger that there might be some work to do
     * This is an ASYNCHRONOUS call.
     */
    virtual void signal() const = 0;
};

// ----------------------------------------------------------------------------

class BnSurfaceComposer : public BnInterface<ISurfaceComposer>
{
public:
    enum {
        // Note: BOOT_FINISHED must remain this value, it is called from
        // Java by ActivityManagerService.
        BOOT_FINISHED = IBinder::FIRST_CALL_TRANSACTION,
        CREATE_CONNECTION,
        CREATE_CLIENT_CONNECTION,
        GET_CBLK,
        OPEN_GLOBAL_TRANSACTION,
        CLOSE_GLOBAL_TRANSACTION,
        SET_ORIENTATION,
        FREEZE_DISPLAY,
        UNFREEZE_DISPLAY,
        SIGNAL,
        CAPTURE_SCREEN,
        TURN_ELECTRON_BEAM_OFF,
        TURN_ELECTRON_BEAM_ON,
        CAPTURE_VIDEO_LAYER,
        DUMP_LAYER,
    };

    virtual status_t    onTransact( uint32_t code,
                                    const Parcel& data,
                                    Parcel* reply,
                                    uint32_t flags = 0);
};

// ----------------------------------------------------------------------------

}; // namespace android

#endif // ANDROID_SF_ISURFACE_COMPOSER_H
