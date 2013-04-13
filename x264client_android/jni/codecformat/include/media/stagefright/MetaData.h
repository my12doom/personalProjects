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
 * Copyright (C) 2009 The Android Open Source Project
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

#ifndef META_DATA_H_

#define META_DATA_H_

#include <sys/types.h>

#include <stdint.h>

#include <utils/RefBase.h>
#include <utils/KeyedVector.h>

namespace android {

// The following keys map to int32_t data unless indicated otherwise.
enum {
    kKeyMIMEType          = 'mime',  // cstring
    kKeyWidth             = 'widt',  // int32_t
    kKeyHeight            = 'heig',  // int32_t
    kKeyRotation          = 'rotA',  // int32_t (angle in degrees)
    kKeyIFramesInterval   = 'ifiv',  // int32_t
    kKeyStride            = 'strd',  // int32_t
    kKeySliceHeight       = 'slht',  // int32_t
    kKeyChannelCount      = '#chn',  // int32_t
    kKeySampleRate        = 'srte',  // int32_t (also video frame rate)
    kKeyBitRate           = 'brte',  // int32_t (bps)
    kKeyESDS              = 'esds',  // raw data
    kKeyAVCC              = 'avcc',  // raw data
    kKeyVorbisInfo        = 'vinf',  // raw data
    kKeyVorbisBooks       = 'vboo',  // raw data
    kKeyWantsNALFragments = 'NALf',
    kKeyIsSyncFrame       = 'sync',  // int32_t (bool)
    kKeyIsCodecConfig     = 'conf',  // int32_t (bool)
    kKeyTime              = 'time',  // int64_t (usecs)
    kKeyNTPTime           = 'ntpT',  // uint64_t (ntp-timestamp)
    kKeyTargetTime        = 'tarT',  // int64_t (usecs)
    kKeyDriftTime         = 'dftT',  // int64_t (usecs)
    kKeyAnchorTime        = 'ancT',  // int64_t (usecs)
    kKeyDuration          = 'dura',  // int64_t (usecs)
    kKeyColorFormat       = 'colf',
    kKeyPlatformPrivate   = 'priv',  // pointer
    kKeyDecoderComponent  = 'decC',  // cstring
    kKeyBufferID          = 'bfID',
    kKeyMaxInputSize      = 'inpS',
    kKeyThumbnailTime     = 'thbT',  // int64_t (usecs)
#ifndef ANDROID_DEFAULT_CODE  
    // Morris Yang add for WMA/WMV
    kKeyWMAC              = 'wmac',  // wma codec specific data
    kKeyWMVC              = 'wmvc',  // wmv codec specific data
#endif // #ifndef ANDROID_DEFAULT_CODE
#ifdef MTK_DRM_APP
    kKeyTrackID           = 'trID',
    kKeyIsDRM             = 'idrm',  // int32_t (bool)
#endif
    kKeyAlbum             = 'albu',  // cstring
    kKeyArtist            = 'arti',  // cstring
    kKeyAlbumArtist       = 'aart',  // cstring
    kKeyComposer          = 'comp',  // cstring
    kKeyGenre             = 'genr',  // cstring
    kKeyTitle             = 'titl',  // cstring
    kKeyYear              = 'year',  // cstring
    kKeyAlbumArt          = 'albA',  // compressed image data
    kKeyAlbumArtMIME      = 'alAM',  // cstring
    kKeyAuthor            = 'auth',  // cstring
    kKeyCDTrackNumber     = 'cdtr',  // cstring
    kKeyDiscNumber        = 'dnum',  // cstring
    kKeyDate              = 'date',  // cstring
    kKeyWriter            = 'writ',  // cstring
    kKeyCompilation       = 'cpil',  // cstring
    kKeyTimeScale         = 'tmsl',  // int32_t

    // video profile and level
    kKeyVideoProfile      = 'vprf',  // int32_t
    kKeyVideoLevel        = 'vlev',  // int32_t

    // Set this key to enable authoring files in 64-bit offset
    kKey64BitFileOffset   = 'fobt',  // int32_t (bool)
    kKey2ByteNalLength    = '2NAL',  // int32_t (bool)

    // Identify the file output format for authoring
    // Please see <media/mediarecorder.h> for the supported
    // file output formats.
    kKeyFileType          = 'ftyp',  // int32_t

    // Track authoring progress status
    // kKeyTrackTimeStatus is used to track progress in elapsed time
    kKeyTrackTimeStatus   = 'tktm',  // int64_t
    kKeyRotationDegree    = 'rdge',  // int32_t (clockwise, in degree)

    kKeyNotRealTime       = 'ntrt',  // bool (int32_t)

    // Ogg files can be tagged to be automatically looping...
    kKeyAutoLoop          = 'autL',  // bool (int32_t)

    kKeyValidSamples      = 'valD',  // int32_t

    kKeyIsUnreadable      = 'unre',  // bool (int32_t)
#ifndef ANDROID_DEFAULT_CODE
	// sam sun for AACExtractor --->
    kKeyCodecConfigInfo   = 'cinf',  // raw data
    kKeyIsAACADIF         = 'adif',  // int32_t (bool)
    // <--- sam sun for AACExtractor
    kKeyAudioPadEnable	  = 'apEn',	 //int32_t(bool),hai.li
    kKeyMaxQueueBuffer    = 'mque',  //int32_t, Demon Deng for OMXCodec
    kKeyAacObjType         = 'aaco',    // Morris Yang for MPEG4 audio object type
    kKeySDP               = 'ksdp',  //int32_t, Demon Deng for SDP
    kKeyServerTimeout     = 'srvt',  //int32_t, Demon Deng for RTSP Server timeout
    kKeyIs3gpBrand		  = '3gpB',  //int32_t(bool), hai.li
    kKeyHasUnsupportVideo = 'UnSV',  //int32_t(bool), hai.li, file has unsupport video track.

#ifdef MTK_AUDIO_APE_SUPPORT
    kkeyComptype            = 'ctyp',   // int16_t compress type
    kkeyApechl              = 'chls',   // int16_t compress type
    kkeyApebit              = 'bits',   // int16_t compress type
    kKeyTotalFrame         = 'alls',  // int32_t all frame in file
    kKeyFinalSample         = 'fins', // int32_t last frame's sample
    kKeySamplesperframe      = 'sapf', // int32_t samples per frame
    kKeyBufferSize            = 'bufs',  //int32_t buffer size for ape
    kKeyNemFrame            = 'nfrm',  //int32_t seek frame's numbers for ape
    kKeySeekByte            = 'sekB',  //int32_t new seek first byte  for ape
#endif

#endif  //#ifndef ANDROID_DEFAULT_CODE
};

enum {
    kTypeESDS        = 'esds',
    kTypeAVCC        = 'avcc',
};

class MetaData : public RefBase {
public:
    MetaData();
    MetaData(const MetaData &from);

    enum Type {
        TYPE_NONE     = 'none',
        TYPE_C_STRING = 'cstr',
        TYPE_INT32    = 'in32',
        TYPE_INT64    = 'in64',
        TYPE_FLOAT    = 'floa',
        TYPE_POINTER  = 'ptr ',
    };

    void clear();
    bool remove(uint32_t key);

    bool setCString(uint32_t key, const char *value);
    bool setInt32(uint32_t key, int32_t value);
    bool setInt64(uint32_t key, int64_t value);
    bool setFloat(uint32_t key, float value);
    bool setPointer(uint32_t key, void *value);

    bool findCString(uint32_t key, const char **value);
    bool findInt32(uint32_t key, int32_t *value);
    bool findInt64(uint32_t key, int64_t *value);
    bool findFloat(uint32_t key, float *value);
    bool findPointer(uint32_t key, void **value);

    bool setData(uint32_t key, uint32_t type, const void *data, size_t size);

    bool findData(uint32_t key, uint32_t *type,
                  const void **data, size_t *size) const;

protected:
    virtual ~MetaData();

private:
    struct typed_data {
        typed_data();
        ~typed_data();

        typed_data(const MetaData::typed_data &);
        typed_data &operator=(const MetaData::typed_data &);

        void clear();
        void setData(uint32_t type, const void *data, size_t size);
        void getData(uint32_t *type, const void **data, size_t *size) const;

    private:
        uint32_t mType;
        size_t mSize;

        union {
            void *ext_data;
            float reservoir;
        } u;

        bool usesReservoir() const {
            return mSize <= sizeof(u.reservoir);
        }

        void allocateStorage(size_t size);
        void freeStorage();

        void *storage() {
            return usesReservoir() ? &u.reservoir : u.ext_data;
        }

        const void *storage() const {
            return usesReservoir() ? &u.reservoir : u.ext_data;
        }
    };

    KeyedVector<uint32_t, typed_data> mItems;

    // MetaData &operator=(const MetaData &);
};

}  // namespace android

#endif  // META_DATA_H_
