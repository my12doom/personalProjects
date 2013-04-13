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

#ifndef ANDROID_APPCCTRL_H_
#define ANDROID_APPCCTRL_H_

#include <utils/RefBase.h>
#include <utils/threads.h>

#include <media/AudioTrack.h>

#include <media/appc_exp.h>
#include <media/AudioPostProcess.h>

extern "C" {
#include <BesSound_exp.h>
#include <media/bli_exp.h>
}


///#include <media/IAPPCService.h>
///#include <media/IAPPCClient.h>

enum APPCAUDIOEFFECT
{
    Effect_Normal,
    Effect_3DSurround,
    Effect_Bass,
    Effect_Concert,
    Effect_Shower,
    Effect_Opera,
    Effect_Dance,
    Effect_Classical,
    Effect_Treble,
    Effect_Party,
    Effect_Pop,
    Effect_Rock,
    Effect_Loudness,
    // blow use for pripritary use.
    Effect_BHPH
};

namespace android {


class APPCDescriptor {
 public:
#if 0 	
	 APPCDescriptor()
	 : Track(0), mAudioAppc(0), pSrcHdl(0){}
#endif
	 // for src convert
	 int				 mSrcSampleRate;
	 uint32_t			 mSrcWorkBufSize;
	 uint32_t			 mSrcDataBufSize;
	 char				 *mSrcWorkBuf;
	 char				 *mSrcDataTempBuf;
	 char				 *mSrcDataBuf;
	 unsigned int		 mSrcWriteIdx;
	 unsigned int		 mSrcTempWriteIdx;
	 BLI_HANDLE 		 *pSrcHdl;
	 
	 // add by chipeng
	 AudioPostProcessing *mAudioAppc;
	 AudioTrack 		 *Track;
	 int				 mSampleRate;		 
	 int				 streamType;
	 int				 sampleRate;
	 int				 format;
	 int				 channels;
	 int				 mFrameCount;

	 int				 mAudioEffectEnable;
	 int				 mAudioEffect;
	 // check for audio post processing is enable.
	 bool				 mAudioAppcEnable;
	 unsigned int		 APPC_STREAM_INFO_SIZE;
	 
	 short				 *tempoutbuf;
	 short				 *tempinbuf;
	 
	 APPC_Stream_Info	 mTrackInputStream;
	 APPC_Stream_Info	 mTrackOutputStream;
	 
	 // record for this track write counter;
	 int				 WriteCounter;
	 uint32_t			 SamplePerByte;
	 bool				 bAppcRampDown;
	 bool				 bAppcRampUp;
	 bool				 bEffectChange;
	 bool				 bHeadsetState;
 };


class APPCCtrl
{
public:

   ///APPCCtrl();
   ///~APPCCtrl();
   
   ////audio_io_handle_t  AudioTrack

   static DefaultKeyedVector<uint32_t, APPCDescriptor *> gAppcmap;

   static void RegisterAPPClient(AudioTrack     *track_h,  
                                int         streamType,
                                uint32_t    sampleRate,
                                int         format,
                                int         channels,
                                int         mFrameCount
                            );

    static  void UnRegisterAPPClient(AudioTrack  *track_h);
    static  APPCDescriptor *GetAppcDesHandle(AudioTrack *track_h);
    
    static void AudioTrackRampDown(void* Buffer, int size, int mFormat, int channelcnt, int SrcRate);   
    static void AudioTrackRampUp(void* Buffer, int size, int mFormat, int channelcnt, int SrcRate);
    static void NeedAdjustAppcBuffer(AudioTrack *track_h, size_t userSize);
    static unsigned int APPCBLI_Convert(void *hdl,  /* Input, handle of this conversion */
                                 short *inBuf,               /* Input, pointer to input buffer */
                                 unsigned int *inLength,     /* Input, length(byte) of input buffer */ 
                                                             /* Output, length(byte) left in the input buffer after conversion */ 
                                 short *outBuf,              /* Input, pointer to output buffer */
                                 unsigned int *outLength);   /* Input, length(byte) of output buffer */ 
                                                             /* Output, output data length(byte) */ 

	static bool TrackNeedSrc(uint8_t StremType,uint32_t samplerate);	 
	static bool NeedLoudnessEnhance(uint8_t StremType, int mSampleRate);
	static bool TrackHasSrc(int mSrcSampleRate);

#if 0
   // APPCCtrl interface ============================================================
   static int SetAudioEffect(int Enable);
   static int GetAudioEffect();
   static int SetAudioEffectParameters(int effect);
   static int GetAudioEffectParameters();

   // register a current process for audio output change notifications
   void registerClient(const sp<IAPPCClient>& client) ;
   void UnregisterClient(const sp<IAPPCClient>& client);

   // helper function to obtain ATVCtrl service handle
   static const sp<IAPPCService>& get_APPCService();
#endif

private:

   static Mutex  mLock;
///   static sp<IAPPCService> spAPPCService;
   static Mutex mAPPCLock;

};

};  // namespace android

#endif  /*ANDROID_APPCCTRL_H_*/

