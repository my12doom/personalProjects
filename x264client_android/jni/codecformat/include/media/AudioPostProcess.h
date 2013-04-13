/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2010. All rights reserved.
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

/*****************************************************************************
*  Copyright Statement:
*  --------------------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of MediaTek Inc. (C) 2008
*
*  BY OPENING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
*  THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
*  RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO BUYER ON
*  AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
*  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
*  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
*  NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
*  SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
*  SUPPLIED WITH THE MEDIATEK SOFTWARE, AND BUYER AGREES TO LOOK ONLY TO SUCH
*  THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. MEDIATEK SHALL ALSO
*  NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE RELEASES MADE TO BUYER'S
*  SPECIFICATION OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
*
*  BUYER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND CUMULATIVE
*  LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
*  AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
*  OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY BUYER TO
*  MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
*
*  THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE
*  WITH THE LAWS OF THE STATE OF CALIFORNIA, USA, EXCLUDING ITS CONFLICT OF
*  LAWS PRINCIPLES.  ANY DISPUTES, CONTROVERSIES OR CLAIMS ARISING THEREOF AND
*  RELATED THERETO SHALL BE SETTLED BY ARBITRATION IN SAN FRANCISCO, CA, UNDER
*  THE RULES OF THE INTERNATIONAL CHAMBER OF COMMERCE (ICC).
*
*****************************************************************************/


/*******************************************************************************
 *
 * Filename:
 * ---------
 * AudioPostProcessing.h
 *
 * Project:
 * --------
 *   Yusu
 *
 * Description:
 * ------------
 *   MT6516 Loudness enhance
 *
 * Author:
 * -------
 *   ChiPeng Chang(mtk02308)
 *
 *------------------------------------------------------------------------------
 * $Revision: #1 $
 * $Modtime:$
 * $Log:$
 *
 * 09 29 2010 chipeng.chang
 * [ALPS00021371] [Need Patch] [Volunteer Patch]Audio Effect integration
 * add AudioPostProcessing and appcservice .
 *
 *******************************************************************************/

#ifndef ANDROID_AUDIO_POSTPROCESS_H
#define ANDROID_AUDIO_POSTPROCESS_H

#include <stdint.h>
#include <sys/types.h>
#include <cutils/log.h>
#include <stdio.h>

#include <media/bs_wrapper_exp.h>
#include <AudioEffectParam.h>


//#define DUMP_INPUTSTREAM
//#define DUMP_OUTPUTSTREAM


namespace android {
// ----------------------------------------------------------------------------
class AudioYusuHardware;

const short Normal_Gain_Db_level[8] =   {  0, 0,  0,  0 ,0,  0, 0, 0};
const short Dance_Gain_Db_level[8]  =    {  16,  64,   0,   8,  32,   40,   32,   16};
const short Bass_Gain_Db_level[8]   =     {  48,  32,  24,  16,   0,    0,    0,    0};
const short Classical_Gain_dB_level[8] = {  40,  24,   0, -16,  -8,    0,   24,   32};
const short Treble_Gain_dB_level[8] =    {   0,   0,   0,   0,   8,   24,   40,   48};
const short Party_Gain_dB_level[8] =      {  40,  32,   0,   0,   0,    0,    0,   32};
const short Pop_Gain_dB_level[8] =        { -12,   0,   8,  40,  40,    8,   -8,  -16};
const short Rock_Gain_dB_level[8] =      {  48,  16,   8,  -8, -16,    8,   24,   40};



static unsigned int BES_EFFECT_HSF[9][4] = {
{	0x00000000, 0x00000000, 0x00000000, 0x00000000	},  /*    48000 */
{	0x00000000, 0x00000000, 0x00000000, 0x00000000	},  /*    44100 */
{	0x00000000, 0x00000000, 0x00000000, 0x00000000	},  /*    32000 */
{	0x00000000, 0x00000000, 0x00000000, 0x00000000	},  /*    24000 */
{	0x00000000, 0x00000000, 0x00000000, 0x00000000	},  /*    22050 */
{	0x00000000, 0x00000000, 0x00000000, 0x00000000	},  /*    16000 */
{	0x00000000, 0x00000000, 0x00000000, 0x00000000	},  /*    12000 */
{	0x00000000, 0x00000000, 0x00000000, 0x00000000	},  /*    11025 */
{	0x00000000, 0x00000000, 0x00000000, 0x00000000	},  /*     8000 */
};

static unsigned int BES_EFFECT_BPF[4][6][3] = {
/* filter 0 */
{
{	0x00000000, 0x00000000, 0x00000000	},  /*    48000 */
{	0x00000000, 0x00000000, 0x00000000	},  /*    44100 */
{	0x00000000, 0x00000000, 0x00000000	},  /*    32000 */
{	0x00000000, 0x00000000, 0x00000000	},  /*    24000 */
{	0x00000000, 0x00000000, 0x00000000	},  /*    22050 */
{	0x00000000, 0x00000000, 0x00000000	},  /*    16000 */
},
/* filter 1 */
{
{	0x00000000, 0x00000000, 0x00000000	},  /*    48000 */
{	0x00000000, 0x00000000, 0x00000000	},  /*    44100 */
{	0x00000000, 0x00000000, 0x00000000	},  /*    32000 */
{	0x00000000, 0x00000000, 0x00000000	},  /*    24000 */
{	0x00000000, 0x00000000, 0x00000000	},  /*    22050 */
{	0x00000000, 0x00000000, 0x00000000	},  /*    16000 */
},
/* filter 2 */
{
{	0x00000000, 0x00000000, 0x00000000	},  /*    48000 */
{	0x00000000, 0x00000000, 0x00000000	},  /*    44100 */
{	0x00000000, 0x00000000, 0x00000000	},  /*    32000 */
{	0x00000000, 0x00000000, 0x00000000	},  /*    24000 */
{	0x00000000, 0x00000000, 0x00000000	},  /*    22050 */
{	0x00000000, 0x00000000, 0x00000000	},  /*    16000 */
},
/* filter 3 */
{
{	0x00000000, 0x00000000, 0x00000000	},  /*    48000 */
{	0x00000000, 0x00000000, 0x00000000	},  /*    44100 */
{	0x00000000, 0x00000000, 0x00000000	},  /*    32000 */
{	0x00000000, 0x00000000, 0x00000000	},  /*    24000 */
{	0x00000000, 0x00000000, 0x00000000	},  /*    22050 */
{	0x00000000, 0x00000000, 0x00000000	},  /*    16000 */
},
};
static unsigned int BES_EFFECT_DRC_FORGET_TABLE[9][2] = {
{	0x00000000, 0x00000000	},
{	0x00000000, 0x00000000	},
{	0x00000000, 0x00000000	},
{	0x00000000, 0x00000000	},
{	0x00000000, 0x00000000	},
{	0x00000000, 0x00000000	},
{	0x00000000, 0x00000000	},
{	0x00000000, 0x00000000	},
{	0x00000000, 0x00000000	},
};

static unsigned int  BES_EFFECT_WS_GAIN_MAX = 0;

static unsigned int  BES_EFFECT_WS_GAIN_MIN = 0;

static unsigned int  BES_EFFECT_FILTER_FIRST = 0;

static char  BES_EFFECT_GAIN_MAP_IN[5] = {
	0, 0, 0, 0, 0,
};
static char  BES_EFFECT_GAIN_MAP_OUT[5] = {
	0, 0, 0, 0, 0,
};



/*
(1): for earphone, sim to iPod
{   0,   0,   0,   0,   0,    0,    0,    0},         Normal
{  48,  32,  24,  16,   0,    0,    0,    0},     Bass
{  16,  64,   0,   8,  32,   40,   32,   16},   Dance
{  40,  24,   0, -16,  -8,    0,   24,   32},  Classical
{   0,   0,   0,   0,   8,   24,   40,   48},      Treble
{  40,  32,   0,   0,   0,    0,    0,   32},      Party
{ -12,   0,   8,  40,  40,    8,   -8,  -16},  Pop
{  48,  16,   8,  -8, -16,    8,   24,   40}   Rock

(2)  : for loud speaker , sim to WinAmp (more obvious)
{   0,   0,   0,   0,   0,    0,    0,    0},                Normal
{  56,  48,  48,  32,  -8,  -32,  -40,  -56},     Bass
{  56,  48, -24, -24,  40,   56,   32,   16},      Dance
{   0,   0,   0,   0,   0,  -32,  -48,  -72},          Classical
{ -56, -16, -24, -16,  -8,   24,   64,   72},    Treble
{  56,  48,   0,   0,   0,    0,    0,   56},             Party
{ -24,  24,  48,  48,  24,  -24,  -32,  -32},    Pop
{  40,  24, -40, -48, -24,    0,   56,   64}       Rock
*/

class AudioPostProcessing
{
public:
    AudioPostProcessing();
    ~AudioPostProcessing();
    void SetEffectEnable(bool Enable);
    void SetAudioEffect(int effect);

    void SetInputStream(APPC_Stream_Info *InputStream);
    void SetOutputStream(APPC_Stream_Info *InputStream);
    bool init();
    bool Deinit();

    void NotifyReset();
    int Process();


    // buffer management API
    int WriteInputStream16bits(short* buffer , size_t size);
    int WriteInputStream8bits(char* buffer , size_t size);
    unsigned int GetInputBufFree(void);
    unsigned int GetInputBufFilled(void);
    unsigned int GetOutputBufAvail(void);
    unsigned int GetOutputBufData(short* buffer , size_t size);

    static AUDIO_EFFECT_CUSTOM_PARAM_STRUCT mAudioEffectParam;
	bool GetStopState(void);
	void SetStopState(bool stop);

private:
    APPC_Handle *mAppcConponent;
    APPC_PostProcess *mpPostProcess;
    AudioEffectInstance* mAudioEffectInstance;
    size_t m_SizeMand;
    size_t m_SizeTemp;
    char* pMand;
    char* pTemp;
    int mAudioeffect;
    bool mEffectEnable;
    APPC_Stream_Info *mOutputStream;
    APPC_Stream_Info *mInputStream;
    volatile WrapperParamBEQ beqParam;

    pthread_mutex_t  mStreamMutex;

#ifdef DUMP_INPUTSTREAM
    FILE  *pInputFile_appc;
#endif


#ifdef DUMP_OUTPUTSTREAM
    FILE  *pOutputFile_appc;
#endif

	bool mTrackStoping;

};

// ----------------------------------------------------------------------------
}; // namespace android

#endif

