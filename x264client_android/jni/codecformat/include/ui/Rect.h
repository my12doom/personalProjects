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

#ifndef ANDROID_UI_RECT
#define ANDROID_UI_RECT

#include <utils/TypeHelpers.h>
#include <ui/Point.h>

#include <android/rect.h>

namespace android {

class Rect : public ARect
{
public:
    typedef int32_t value_type;

    // we don't provide copy-ctor and operator= on purpose
    // because we want the compiler generated versions

    inline Rect() {
    }
    inline Rect(int32_t w, int32_t h) {
        left = top = 0; right = w; bottom = h;
    }
    inline Rect(int32_t l, int32_t t, int32_t r, int32_t b) {
        left = l; top = t; right = r; bottom = b;
    }
    inline Rect(const Point& lt, const Point& rb) {
        left = lt.x; top = lt.y; right = rb.x; bottom = rb.y;
    }

    void makeInvalid();

    inline void clear() {
        left = top = right = bottom = 0;
    }

    // a valid rectangle has a non negative width and height
    inline bool isValid() const {
        return (width()>=0) && (height()>=0);
    }

    // an empty rect has a zero width or height, or is invalid
    inline bool isEmpty() const {
        return (width()<=0) || (height()<=0);
    }

    inline void set(const Rect& rhs) {
        operator = (rhs);
    }

    // rectangle's width
    inline int32_t width() const {
        return right-left;
    }
    
    // rectangle's height
    inline int32_t height() const {
        return bottom-top;
    }

    void setLeftTop(const Point& lt) {
        left = lt.x;
        top  = lt.y;
    }

    void setRightBottom(const Point& rb) {
        right = rb.x;
        bottom  = rb.y;
    }
    
    // the following 4 functions return the 4 corners of the rect as Point
    Point leftTop() const {
        return Point(left, top);
    }
    Point rightBottom() const {
        return Point(right, bottom);
    }
    Point rightTop() const {
        return Point(right, top);
    }
    Point leftBottom() const {
        return Point(left, bottom);
    }

    // comparisons
    inline bool operator == (const Rect& rhs) const {
        return (left == rhs.left) && (top == rhs.top) &&
               (right == rhs.right) && (bottom == rhs.bottom);
    }

    inline bool operator != (const Rect& rhs) const {
        return !operator == (rhs);
    }

    // operator < defines an order which allows to use rectangles in sorted
    // vectors.
    bool operator < (const Rect& rhs) const;

    Rect& offsetToOrigin() {
        right -= left;
        bottom -= top;
        left = top = 0;
        return *this;
    }
    Rect& offsetTo(const Point& p) {
        return offsetTo(p.x, p.y);
    }
    Rect& offsetBy(const Point& dp) {
        return offsetBy(dp.x, dp.y);
    }
    Rect& operator += (const Point& rhs) {
        return offsetBy(rhs.x, rhs.y);
    }
    Rect& operator -= (const Point& rhs) {
        return offsetBy(-rhs.x, -rhs.y);
    }
    const Rect operator + (const Point& rhs) const;
    const Rect operator - (const Point& rhs) const;

    void translate(int32_t dx, int32_t dy) { // legacy, don't use.
        offsetBy(dx, dy);
    }
 
    Rect&   offsetTo(int32_t x, int32_t y);
    Rect&   offsetBy(int32_t x, int32_t y);
    bool    intersect(const Rect& with, Rect* result) const;
};

ANDROID_BASIC_TYPES_TRAITS(Rect)

}; // namespace android

#endif // ANDROID_UI_RECT
