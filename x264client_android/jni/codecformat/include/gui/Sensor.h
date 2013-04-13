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

#ifndef ANDROID_GUI_SENSOR_H
#define ANDROID_GUI_SENSOR_H

#include <stdint.h>
#include <sys/types.h>

#include <utils/Errors.h>
#include <utils/String8.h>
#include <utils/Flattenable.h>

#include <hardware/sensors.h>

#include <android/sensor.h>

// ----------------------------------------------------------------------------
// Concrete types for the NDK
struct ASensor { };

// ----------------------------------------------------------------------------
namespace android {
// ----------------------------------------------------------------------------

class Parcel;

// ----------------------------------------------------------------------------

class Sensor : public ASensor, public Flattenable
{
public:
    enum {
        TYPE_ACCELEROMETER  = ASENSOR_TYPE_ACCELEROMETER,
        TYPE_MAGNETIC_FIELD = ASENSOR_TYPE_MAGNETIC_FIELD,
        TYPE_GYROSCOPE      = ASENSOR_TYPE_GYROSCOPE,
        TYPE_LIGHT          = ASENSOR_TYPE_LIGHT,
        TYPE_PROXIMITY      = ASENSOR_TYPE_PROXIMITY
    };

            Sensor();
            Sensor(struct sensor_t const* hwSensor);
    virtual ~Sensor();

    const String8& getName() const;
    const String8& getVendor() const;
    int32_t getHandle() const;
    int32_t getType() const;
    float getMinValue() const;
    float getMaxValue() const;
    float getResolution() const;
    float getPowerUsage() const;
    int32_t getMinDelay() const;

    // Flattenable interface
    virtual size_t getFlattenedSize() const;
    virtual size_t getFdCount() const;
    virtual status_t flatten(void* buffer, size_t size,
            int fds[], size_t count) const;
    virtual status_t unflatten(void const* buffer, size_t size,
            int fds[], size_t count);

private:
    String8 mName;
    String8 mVendor;
    int32_t mHandle;
    int32_t mType;
    float   mMinValue;
    float   mMaxValue;
    float   mResolution;
    float   mPower;
    int32_t mMinDelay;
};

// ----------------------------------------------------------------------------
}; // namespace android

#endif // ANDROID_GUI_SENSOR_H
