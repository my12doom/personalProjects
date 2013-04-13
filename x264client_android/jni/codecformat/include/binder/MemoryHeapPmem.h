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
 * Copyright (C) 2008 The Android Open Source Project
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

#ifndef ANDROID_MEMORY_HEAP_PMEM_H
#define ANDROID_MEMORY_HEAP_PMEM_H

#include <stdlib.h>
#include <stdint.h>

#include <binder/MemoryHeapBase.h>
#include <binder/IMemory.h>
#include <utils/SortedVector.h>
#include <utils/threads.h>

namespace android {

class MemoryHeapBase;

// ---------------------------------------------------------------------------

class MemoryHeapPmem : public MemoryHeapBase
{
public:
    class MemoryPmem : public BnMemory {
    public:
        MemoryPmem(const sp<MemoryHeapPmem>& heap);
        ~MemoryPmem();
    protected:
        const sp<MemoryHeapPmem>&  getHeap() const { return mClientHeap; }
    private:
        friend class MemoryHeapPmem;
        virtual void revoke() = 0;
        sp<MemoryHeapPmem>  mClientHeap;
    };
    
    MemoryHeapPmem(const sp<MemoryHeapBase>& pmemHeap, uint32_t flags = 0);
    ~MemoryHeapPmem();

    /* HeapInterface additions */
    virtual sp<IMemory> mapMemory(size_t offset, size_t size);

    /* make the whole heap visible (you know who you are) */
    virtual status_t slap();
    
    /* hide (revoke) the whole heap (the client will see the garbage page) */
    virtual status_t unslap();
    
    /* revoke all allocations made by this heap */
    virtual void revoke();

private:
    /* use this to create your own IMemory for mapMemory */
    virtual sp<MemoryPmem> createMemory(size_t offset, size_t size);
    void remove(const wp<MemoryPmem>& memory);

private:
    sp<MemoryHeapBase>              mParentHeap;
    mutable Mutex                   mLock;
    SortedVector< wp<MemoryPmem> >  mAllocations;
};


// ---------------------------------------------------------------------------
}; // namespace android

#endif // ANDROID_MEMORY_HEAP_PMEM_H
