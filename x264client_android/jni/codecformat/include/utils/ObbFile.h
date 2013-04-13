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

#ifndef OBBFILE_H_
#define OBBFILE_H_

#include <stdint.h>
#include <strings.h>

#include <utils/RefBase.h>
#include <utils/String8.h>

namespace android {

// OBB flags (bit 0)
#define OBB_OVERLAY         (1 << 0)
#define OBB_SALTED          (1 << 1)

class ObbFile : public RefBase {
protected:
    virtual ~ObbFile();

public:
    ObbFile();

    bool readFrom(const char* filename);
    bool readFrom(int fd);
    bool writeTo(const char* filename);
    bool writeTo(int fd);
    bool removeFrom(const char* filename);
    bool removeFrom(int fd);

    const char* getFileName() const {
        return mFileName;
    }

    const String8 getPackageName() const {
        return mPackageName;
    }

    void setPackageName(String8 packageName) {
        mPackageName = packageName;
    }

    int32_t getVersion() const {
        return mVersion;
    }

    void setVersion(int32_t version) {
        mVersion = version;
    }

    int32_t getFlags() const {
        return mFlags;
    }

    void setFlags(int32_t flags) {
        mFlags = flags;
    }

    const unsigned char* getSalt(size_t* length) const {
        if ((mFlags & OBB_SALTED) == 0) {
            *length = 0;
            return NULL;
        }

        *length = sizeof(mSalt);
        return mSalt;
    }

    bool setSalt(const unsigned char* salt, size_t length) {
        if (length != sizeof(mSalt)) {
            return false;
        }

        memcpy(mSalt, salt, sizeof(mSalt));
        mFlags |= OBB_SALTED;
        return true;
    }

    bool isOverlay() {
        return (mFlags & OBB_OVERLAY) == OBB_OVERLAY;
    }

    void setOverlay(bool overlay) {
        if (overlay) {
            mFlags |= OBB_OVERLAY;
        } else {
            mFlags &= ~OBB_OVERLAY;
        }
    }

    static inline uint32_t get4LE(const unsigned char* buf) {
        return buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
    }

    static inline void put4LE(unsigned char* buf, uint32_t val) {
        buf[0] = val & 0xFF;
        buf[1] = (val >> 8) & 0xFF;
        buf[2] = (val >> 16) & 0xFF;
        buf[3] = (val >> 24) & 0xFF;
    }

private:
    /* Package name this ObbFile is associated with */
    String8 mPackageName;

    /* Package version this ObbFile is associated with */
    int32_t mVersion;

    /* Flags for this OBB type. */
    int32_t mFlags;

    /* Whether the file is salted. */
    bool mSalted;

    /* The encryption salt. */
    unsigned char mSalt[8];

    const char* mFileName;

    size_t mFileSize;

    size_t mFooterStart;

    unsigned char* mReadBuf;

    bool parseObbFile(int fd);
};

}
#endif /* OBBFILE_H_ */
