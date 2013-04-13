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

#ifndef _UI_INPUT_MANAGER_H
#define _UI_INPUT_MANAGER_H

/**
 * Native input manager.
 */

#include <ui/EventHub.h>
#include <ui/Input.h>
#include <utils/Errors.h>
#include <utils/Vector.h>
#include <utils/Timers.h>
#include <utils/RefBase.h>
#include <utils/String8.h>

namespace android {

class InputChannel;

class InputReaderInterface;
class InputReaderPolicyInterface;
class InputReaderThread;

class InputDispatcherInterface;
class InputDispatcherPolicyInterface;
class InputDispatcherThread;

/*
 * The input manager is the core of the system event processing.
 *
 * The input manager uses two threads.
 *
 * 1. The InputReaderThread (called "InputReader") reads and preprocesses raw input events,
 *    applies policy, and posts messages to a queue managed by the DispatcherThread.
 * 2. The InputDispatcherThread (called "InputDispatcher") thread waits for new events on the
 *    queue and asynchronously dispatches them to applications.
 *
 * By design, the InputReaderThread class and InputDispatcherThread class do not share any
 * internal state.  Moreover, all communication is done one way from the InputReaderThread
 * into the InputDispatcherThread and never the reverse.  Both classes may interact with the
 * InputDispatchPolicy, however.
 *
 * The InputManager class never makes any calls into Java itself.  Instead, the
 * InputDispatchPolicy is responsible for performing all external interactions with the
 * system, including calling DVM services.
 */
class InputManagerInterface : public virtual RefBase {
protected:
    InputManagerInterface() { }
    virtual ~InputManagerInterface() { }

public:
    /* Starts the input manager threads. */
    virtual status_t start() = 0;

    /* Stops the input manager threads and waits for them to exit. */
    virtual status_t stop() = 0;

    /* Gets the input reader. */
    virtual sp<InputReaderInterface> getReader() = 0;

    /* Gets the input dispatcher. */
    virtual sp<InputDispatcherInterface> getDispatcher() = 0;
};

class InputManager : public InputManagerInterface {
protected:
    virtual ~InputManager();

public:
    InputManager(
            const sp<EventHubInterface>& eventHub,
            const sp<InputReaderPolicyInterface>& readerPolicy,
            const sp<InputDispatcherPolicyInterface>& dispatcherPolicy);

    // (used for testing purposes)
    InputManager(
            const sp<InputReaderInterface>& reader,
            const sp<InputDispatcherInterface>& dispatcher);

    virtual status_t start();
    virtual status_t stop();

    virtual sp<InputReaderInterface> getReader();
    virtual sp<InputDispatcherInterface> getDispatcher();

private:
    sp<InputReaderInterface> mReader;
    sp<InputReaderThread> mReaderThread;

    sp<InputDispatcherInterface> mDispatcher;
    sp<InputDispatcherThread> mDispatcherThread;

    void initialize();
};

} // namespace android

#endif // _UI_INPUT_MANAGER_H
