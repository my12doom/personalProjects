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

//
// Access a chunk of the asset hierarchy as if it were a single directory.
//
#ifndef __LIBS_ASSETDIR_H
#define __LIBS_ASSETDIR_H

#include <utils/String8.h>
#include <utils/Vector.h>
#include <utils/SortedVector.h>
#include <utils/misc.h>
#include <sys/types.h>

namespace android {

/*
 * This provides vector-style access to a directory.  We do this rather
 * than modeling opendir/readdir access because it's simpler and the
 * nature of the operation requires us to have all data on hand anyway.
 *
 * The list of files will be sorted in ascending order by ASCII value.
 *
 * The contents are populated by our friend, the AssetManager.
 */
class AssetDir {
public:
    AssetDir(void)
        : mFileInfo(NULL)
        {}
    virtual ~AssetDir(void) {
        delete mFileInfo;
    }

    /*
     * Vector-style access.
     */
    size_t getFileCount(void) { return mFileInfo->size(); }
    const String8& getFileName(int idx) {
        return mFileInfo->itemAt(idx).getFileName();
    }
    const String8& getSourceName(int idx) {
        return mFileInfo->itemAt(idx).getSourceName();
    }

    /*
     * Get the type of a file (usually regular or directory).
     */
    FileType getFileType(int idx) {
        return mFileInfo->itemAt(idx).getFileType();
    }

private:
    /* these operations are not implemented */
    AssetDir(const AssetDir& src);
    const AssetDir& operator=(const AssetDir& src);

    friend class AssetManager;

    /*
     * This holds information about files in the asset hierarchy.
     */
    class FileInfo {
    public:
        FileInfo(void) {}
        FileInfo(const String8& path)      // useful for e.g. svect.indexOf
            : mFileName(path), mFileType(kFileTypeUnknown)
            {}
        ~FileInfo(void) {}
        FileInfo(const FileInfo& src) {
            copyMembers(src);
        }
        const FileInfo& operator= (const FileInfo& src) {
            if (this != &src)
                copyMembers(src);
            return *this;
        }

        void copyMembers(const FileInfo& src) {
            mFileName = src.mFileName;
            mFileType = src.mFileType;
            mSourceName = src.mSourceName;
        }

        /* need this for SortedVector; must compare only on file name */
        bool operator< (const FileInfo& rhs) const {
            return mFileName < rhs.mFileName;
        }

        /* used by AssetManager */
        bool operator== (const FileInfo& rhs) const {
            return mFileName == rhs.mFileName;
        }

        void set(const String8& path, FileType type) {
            mFileName = path;
            mFileType = type;
        }

        const String8& getFileName(void) const { return mFileName; }
        void setFileName(const String8& path) { mFileName = path; }

        FileType getFileType(void) const { return mFileType; }
        void setFileType(FileType type) { mFileType = type; }

        const String8& getSourceName(void) const { return mSourceName; }
        void setSourceName(const String8& path) { mSourceName = path; }

        /*
         * Handy utility for finding an entry in a sorted vector of FileInfo.
         * Returns the index of the matching entry, or -1 if none found.
         */
        static int findEntry(const SortedVector<FileInfo>* pVector,
            const String8& fileName);

    private:
        String8    mFileName;      // filename only
        FileType    mFileType;      // regular, directory, etc

        String8    mSourceName;    // currently debug-only
    };

    /* AssetManager uses this to initialize us */
    void setFileList(SortedVector<FileInfo>* list) { mFileInfo = list; }

    SortedVector<FileInfo>* mFileInfo;
};

}; // namespace android

#endif // __LIBS_ASSETDIR_H
