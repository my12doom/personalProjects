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

#ifndef ANDROID_IMOUNTSERVICE_H
#define ANDROID_IMOUNTSERVICE_H

#include <storage/IMountServiceListener.h>
#include <storage/IMountShutdownObserver.h>
#include <storage/IObbActionListener.h>

#include <binder/IInterface.h>
#include <binder/Parcel.h>

namespace android {

class IMountService: public IInterface {
public:
    DECLARE_META_INTERFACE(MountService);

    virtual void registerListener(const sp<IMountServiceListener>& listener) = 0;
    virtual void
            unregisterListener(const sp<IMountServiceListener>& listener) = 0;
    virtual bool isUsbMassStorageConnected() = 0;
    virtual void setUsbMassStorageEnabled(const bool enable) = 0;
    virtual bool isUsbMassStorageEnabled() = 0;
    virtual int32_t mountVolume(const String16& mountPoint) = 0;
    virtual int32_t
            unmountVolume(const String16& mountPoint, const bool force) = 0;
    virtual int32_t formatVolume(const String16& mountPoint) = 0;
    virtual int32_t
            getStorageUsers(const String16& mountPoint, int32_t** users) = 0;
    virtual int32_t getVolumeState(const String16& mountPoint) = 0;
    virtual int32_t createSecureContainer(const String16& id,
            const int32_t sizeMb, const String16& fstype, const String16& key,
            const int32_t ownerUid) = 0;
    virtual int32_t finalizeSecureContainer(const String16& id) = 0;
    virtual int32_t destroySecureContainer(const String16& id) = 0;
    virtual int32_t mountSecureContainer(const String16& id,
            const String16& key, const int32_t ownerUid) = 0;
    virtual int32_t
            unmountSecureContainer(const String16& id, const bool force) = 0;
    virtual bool isSecureContainerMounted(const String16& id) = 0;
    virtual int32_t renameSecureContainer(const String16& oldId,
            const String16& newId) = 0;
    virtual bool getSecureContainerPath(const String16& id, String16& path) = 0;
    virtual int32_t getSecureContainerList(const String16& id,
            String16*& containers) = 0;
    virtual void shutdown(const sp<IMountShutdownObserver>& observer) = 0;
    virtual void finishMediaUpdate() = 0;
    virtual void mountObb(const String16& filename, const String16& key,
            const sp<IObbActionListener>& token, const int32_t nonce) = 0;
    virtual void unmountObb(const String16& filename, const bool force,
            const sp<IObbActionListener>& token, const int32_t nonce) = 0;
    virtual bool isObbMounted(const String16& filename) = 0;
    virtual bool getMountedObbPath(const String16& filename, String16& path) = 0;
};

// ----------------------------------------------------------------------------

class BnMountService: public BnInterface<IMountService> {
public:
    virtual status_t onTransact(uint32_t code, const Parcel& data,
            Parcel* reply, uint32_t flags = 0);
};

}
; // namespace android

#endif // ANDROID_IMOUNTSERVICE_H
