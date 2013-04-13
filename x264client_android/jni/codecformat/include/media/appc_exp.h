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
 *   appc_exp.h
 *
 * Project:
 * --------
 *   
 *
 * Description:
 * ------------
 *   Header file of Audio Post-Processing Component (APPC) for export.
 *   To process PCM data and propagate the processed data to the next component.
 *
 * Author:
 * -------
 *   KH Hung
 *
 *==============================================================================
 *             HISTORY
 * Below this line, this part is controlled by PVCS VM. DO NOT MODIFY!!
 *------------------------------------------------------------------------------
 * $Revision$
 * $Modtime$
 * $Log$
 *
 * 09 29 2010 chipeng.chang
 * [ALPS00021371] [Need Patch] [Volunteer Patch]Audio Effect integration
 * add AudioPostProcessing and appcservice .
 *
 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by PVCS VM. DO NOT MODIFY!!
 *==============================================================================
 *******************************************************************************/

#ifndef _APPC_EXP_H
#define _APPC_EXP_H

/* For C++ portable */
#ifdef __cplusplus
extern "C"{
#endif

/*******************************************************************************/
/* Enum Definition                                                             */
/*******************************************************************************/

/**** Public Report : to return the result of operation. ****/
typedef enum {
   APPC_REPORT_SUCCESS,
   APPC_REPORT_FAIL,
   APPC_REPORT_INVALID_COMPONENT_HANDLE,
   APPC_REPORT_INVALID_POST_PROCESS_HANDLE,
   APPC_REPORT_INVALID_BUFFER
} APPC_Report;

/*******************************************************************************/
/* Structure Definition                                                        */
/*******************************************************************************/

/**** APPC stream information ****/
typedef struct {
   // Sample Information
   unsigned int sampleRate;          /* The sampling rate of output PCM                        */
   unsigned int channelNum;          /* The number of channel of output PCM                    */

   // Buffer Information
   short        *bufBase;            /* The base address of buffer                             */
   unsigned int bufSize;             /* The size of buffer (unit: short)                       */
   unsigned int readOffset;          /* The read position (offset) (unit: short)               */
   unsigned int writeOffset;         /* The write position (offset) (unit: short)              */
} APPC_Stream_Info;

/**** APPC post-process structure ****/
typedef struct appc_post_process APPC_PostProcess;

struct appc_post_process{
   void (*Reset)(APPC_PostProcess *pp,
                 unsigned int     sampleRate,
                 unsigned int     channelNum);
   unsigned int (*GetMemorySize)(APPC_PostProcess *pp);
   void (*SetMemory)(APPC_PostProcess *pp, void *buf);
   void (*Process)(APPC_PostProcess *pp,
                   const short      *pInputBuffer,
                   unsigned int     *InputSampleCount,
                   short            *pOutputBuffer,
                   unsigned int     *OutputSampleCount);
   void (*EndOfStream)(APPC_PostProcess *pp,
                       const short      *pInputBuffer,
                       unsigned int     *InputSampleCount,
                       short            *pOutputBuffer,
                       unsigned int     *OutputSampleCount);
   void (*SetParameters)(APPC_PostProcess *pp, unsigned int byteLen, void *data);
};


/**** APPC handle structure ****/
typedef struct appc_component APPC_Handle;

struct appc_component {
   // State and information
   unsigned int      state;             /* State of component                      */
   unsigned int      isEnable;          /* The component is enabled or not         */
   unsigned int      isReset;           /* The component should be reset           */
   
   // Component connection
   APPC_Handle       *prevComponent;    /* Pointer of input component              */
   APPC_Handle       *nextComponent;    /* Pointer of output component             */
   
   // Post Process
   APPC_PostProcess  *postprocess;      /* Pointer of hooked post-process          */
   void              *ppBufBase;        /* The base address of post-process buffer */
   unsigned int      ppBufSize;         /* The size of post-process buffer */

   // Stream information
   APPC_Stream_Info  *input;            /* The input stream information            */
   APPC_Stream_Info  *output;           /* The output stream information           */

   // Temporary PCM buffer information
   short         *tempBufBase;          /* The base address of temporary buffer              */
   unsigned int  tempBufSize;           /* The size of temporary buffer (unit: short)        */
   // Partition of temporary buffer
   short         *tempInBufBase;        /* The base address of temporary input buffer        */
   unsigned int  tempInBufSize;         /* The size of temporary input buffer (unit: short)  */
   short         *tempOutBufBase;       /* The base address of temporary output buffer       */
   unsigned int  tempOutBufSize;        /* The size of temporary output buffer (unit: short) */

   // Parameter
   unsigned int  bParamSize;            /* The size of parameter (in byte) */
   void          *pParam;               /* The parameter pointer           */
};


/*******************************************************************************/
/* Interface Functions                                                         */
/*******************************************************************************/

/* ---------------------------------------------------------- */
/* To create APPC handle                                      */
/*                                                            */
/* Return:                                                    */
/*    The report and handle for operation.                    */
/* ---------------------------------------------------------- */
APPC_Report APPC_Create(
   APPC_Handle       *hdl,           /* (i/o) the address of component handle */
   APPC_PostProcess  *postprocess    /* (i) the address of post-process       */
);

/* -------------------------------------------------------------- */
/* To query the memory size needed by component and post-process. */
/* The unit is byte.                                              */
/*                                                                */
/* Argument                                                       */
/*   mandBufSize: the static memory needed by                     */
/*            component and post-process (mandatory).             */
/*   tempBufSize: the temporary buffer needed by                  */
/*            component and post-process.                         */
/*                                                                */
/* Return:                                                        */
/*    The report for operation.                                   */
/* -------------------------------------------------------------- */
APPC_Report APPC_QueryMemerySize(
   APPC_Handle   *hdl,          /* (i) the address of component handle              */
   unsigned int  *mandBufSize,  /* (o) the required mandatory memory size (in byte) */
   unsigned int  *tempBufSize   /* (o) the required temporary memory size (in byte) */
);

/* ---------------------------------------------------------- */
/* To assign buffer to component and post-process.            */
/*                                                            */
/* Return:                                                    */
/*    The report for operation.                               */
/* ---------------------------------------------------------- */
APPC_Report APPC_SetBuffer(
   APPC_Handle   *hdl,           /* (i) the address of component handle     */
   void          *mandBuf,       /* (i) the mandatory buffer address        */
   void          *tempBuf        /* (i) the temporary buffer address        */
);

/* ------------------------------------------------------------------------ */
/* To notify for reseting APPC handle, buffer information, and post-process */
/*                                                                          */
/* Return:                                                                  */
/*    The report for operation.                                             */
/*                                                                          */
/* Note:                                                                    */
/*    The function is propagated.                                           */
/* ------------------------------------------------------------------------ */
APPC_Report APPC_NotifyReset(
   APPC_Handle  *hdl            /* (i) the address of component handle */
);

/* ---------------------------------------------------------- */
/* To connect component                                       */
/*    i.e. to establish the link of data flow.                */
/*                                                            */
/* Return:                                                    */
/*    The report for operation.                               */
/* ---------------------------------------------------------- */
APPC_Report APPC_Connect(
   APPC_Handle  *srcHdl,       /* (i) the source of connection      */
   APPC_Handle  *destHdl       /* (i) the destination of connection */
);

/* ---------------------------------------------------------- */
/* To disconnect component                                    */
/*    i.e. to break the link of data flow.                    */
/*                                                            */
/* Return:                                                    */
/*    The report for operation.                               */
/* ---------------------------------------------------------- */
APPC_Report APPC_Disconnect(
   APPC_Handle  *srcHdl,       /* (i) the source of connection      */
   APPC_Handle  *destHdl       /* (i) the destination of connection */
);

/* ---------------------------------------------------------- */
/* To enable or disable the component and its post-process    */
/*                                                            */
/* Return:                                                    */
/*    The report for operation.                               */
/* ---------------------------------------------------------- */
APPC_Report APPC_Enable(
   APPC_Handle   *hdl,           /* (i) the address of component handle */
   unsigned int  isEnable        /* (i) 0: disable; otherwise: enable   */
);

/* ---------------------------------------------------------- */
/* To set the output stream of the component.                 */
/*                                                            */
/* Return:                                                    */
/*    The report for operation.                               */
/* ---------------------------------------------------------- */
APPC_Report APPC_SetOutputStream(
   APPC_Handle       *hdl,           /* (i) the address of component handle */
   APPC_Stream_Info  *outStream      /* (i) the output stream               */
);

/* ---------------------------------------------------------- */
/* To process the input stream and                            */
/*    put the processed on output stream.                     */
/*                                                            */
/* Return:                                                    */
/*    The report for operation.                               */
/* Note:                                                      */
/*    The function is propagated.                             */
/* ---------------------------------------------------------- */
APPC_Report APPC_Process(
   APPC_Handle       *hdl,           /* (i) the address of component handle */
   APPC_Stream_Info  *inStream       /* (i/o) the input stream              */
);

/* ---------------------------------------------------------- */
/* To flush the remained data in each component.              */
/*                                                            */
/* Return:                                                    */
/*    The report for operation.                               */
/* Note:                                                      */
/*    The function is propagated.                             */
/* ---------------------------------------------------------- */
APPC_Report APPC_EndOfStream(
   APPC_Handle       *hdl,           /* (i) the address of component handle */
   APPC_Stream_Info  *inStream       /* (i/o) the input stream              */
);

/* ---------------------------------------------------------- */
/* To set parameters to post-process.                         */
/*                                                            */
/* Return:                                                    */
/*    The report for operation.                               */
/* ---------------------------------------------------------- */
APPC_Report APPC_SetParameter(
   APPC_Handle   *hdl,           /* (i) the address of component handle   */
   unsigned int  byteLen,        /* (i) the length of parameter (in byte) */
   void          *data           /* (i) the address of parameters         */
);


/* For C++ portable */
#ifdef __cplusplus
}
#endif

#endif

