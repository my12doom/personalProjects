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

#ifndef ANDROID_EFFECTENVIRONMENTALREVERBAPI_H_
#define ANDROID_EFFECTENVIRONMENTALREVERBAPI_H_

#include <media/EffectApi.h>

#if __cplusplus
extern "C" {
#endif

#ifndef OPENSL_ES_H_
static const effect_uuid_t SL_IID_ENVIRONMENTALREVERB_ = { 0xc2e5d5f0, 0x94bd, 0x4763, 0x9cac, { 0x4e, 0x23, 0x4d, 0x6, 0x83, 0x9e } };
const effect_uuid_t * const SL_IID_ENVIRONMENTALREVERB = &SL_IID_ENVIRONMENTALREVERB_;
#endif //OPENSL_ES_H_

/* enumerated parameter settings for environmental reverb effect */
typedef enum
{
    // Parameters below are as defined in OpenSL ES specification for environmental reverb interface
    REVERB_PARAM_ROOM_LEVEL,            // in millibels,    range -6000 to 0
    REVERB_PARAM_ROOM_HF_LEVEL,         // in millibels,    range -4000 to 0
    REVERB_PARAM_DECAY_TIME,            // in milliseconds, range 100 to 20000
    REVERB_PARAM_DECAY_HF_RATIO,        // in permilles,    range 100 to 1000
    REVERB_PARAM_REFLECTIONS_LEVEL,     // in millibels,    range -6000 to 0
    REVERB_PARAM_REFLECTIONS_DELAY,     // in milliseconds, range 0 to 65
    REVERB_PARAM_REVERB_LEVEL,          // in millibels,    range -6000 to 0
    REVERB_PARAM_REVERB_DELAY,          // in milliseconds, range 0 to 65
    REVERB_PARAM_DIFFUSION,             // in permilles,    range 0 to 1000
    REVERB_PARAM_DENSITY,               // in permilles,    range 0 to 1000
    REVERB_PARAM_PROPERTIES,
    REVERB_PARAM_BYPASS
} t_env_reverb_params;

//t_reverb_settings is equal to SLEnvironmentalReverbSettings defined in OpenSL ES specification.
typedef struct s_reverb_settings {
    int16_t     roomLevel;
    int16_t     roomHFLevel;
    uint32_t    decayTime;
    int16_t     decayHFRatio;
    int16_t     reflectionsLevel;
    uint32_t    reflectionsDelay;
    int16_t     reverbLevel;
    uint32_t    reverbDelay;
    int16_t     diffusion;
    int16_t     density;
} __attribute__((packed)) t_reverb_settings;


#if __cplusplus
}  // extern "C"
#endif


#endif /*ANDROID_EFFECTENVIRONMENTALREVERBAPI_H_*/
