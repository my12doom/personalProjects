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
 * Copyright (C) 2005 The Android Open Source Project
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

//
#ifndef ANDROID_ISERVICE_MANAGER_H
#define ANDROID_ISERVICE_MANAGER_H

#include <binder/IInterface.h>
#include <binder/IPermissionController.h>
#include <utils/Vector.h>
#include <utils/String16.h>

namespace android {

// ----------------------------------------------------------------------

class IServiceManager : public IInterface
{
public:
    DECLARE_META_INTERFACE(ServiceManager);

    /**
     * Retrieve an existing service, blocking for a few seconds
     * if it doesn't yet exist.
     */
    virtual sp<IBinder>         getService( const String16& name) const = 0;

    /**
     * Retrieve an existing service, non-blocking.
     */
    virtual sp<IBinder>         checkService( const String16& name) const = 0;

    /**
     * Register a service.
     */
    virtual status_t            addService( const String16& name,
                                            const sp<IBinder>& service) = 0;

    /**
     * Return list of all existing services.
     */
    virtual Vector<String16>    listServices() = 0;

    enum {
        GET_SERVICE_TRANSACTION = IBinder::FIRST_CALL_TRANSACTION,
        CHECK_SERVICE_TRANSACTION,
        ADD_SERVICE_TRANSACTION,
        LIST_SERVICES_TRANSACTION,
    };
};

sp<IServiceManager> defaultServiceManager();

template<typename INTERFACE>
status_t getService(const String16& name, sp<INTERFACE>* outService)
{
    const sp<IServiceManager> sm = defaultServiceManager();
    if (sm != NULL) {
        *outService = interface_cast<INTERFACE>(sm->getService(name));
        if ((*outService) != NULL) return NO_ERROR;
    }
    return NAME_NOT_FOUND;
}

bool checkCallingPermission(const String16& permission);
bool checkCallingPermission(const String16& permission,
                            int32_t* outPid, int32_t* outUid);
bool checkPermission(const String16& permission, pid_t pid, uid_t uid);


// ----------------------------------------------------------------------

class BnServiceManager : public BnInterface<IServiceManager>
{
public:
    virtual status_t    onTransact( uint32_t code,
                                    const Parcel& data,
                                    Parcel* reply,
                                    uint32_t flags = 0);
};

// ----------------------------------------------------------------------

}; // namespace android

#endif // ANDROID_ISERVICE_MANAGER_H
