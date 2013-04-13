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
 * Copyright (C) 2005 The Android Open Source Project
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

#ifndef ANDROID_TYPE_HELPERS_H
#define ANDROID_TYPE_HELPERS_H

#include <new>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>

// ---------------------------------------------------------------------------

namespace android {

/*
 * Types traits
 */

template <typename T> struct trait_trivial_ctor { enum { value = false }; };
template <typename T> struct trait_trivial_dtor { enum { value = false }; };
template <typename T> struct trait_trivial_copy { enum { value = false }; };
template <typename T> struct trait_trivial_move { enum { value = false }; };
template <typename T> struct trait_pointer      { enum { value = false }; };    
template <typename T> struct trait_pointer<T*>  { enum { value = true }; };

// sp<> can be trivially moved
template <typename T> class sp;
template <typename T> struct trait_trivial_move< sp<T> >{
    enum { value = true }; 
};

// wp<> can be trivially moved
template <typename T> class wp;
template <typename T> struct trait_trivial_move< wp<T> >{ 
    enum { value = true }; 
};

template <typename TYPE>
struct traits {
    enum {
        // whether this type is a pointer
        is_pointer          = trait_pointer<TYPE>::value,
        // whether this type's constructor is a no-op
        has_trivial_ctor    = is_pointer || trait_trivial_ctor<TYPE>::value,
        // whether this type's destructor is a no-op
        has_trivial_dtor    = is_pointer || trait_trivial_dtor<TYPE>::value,
        // whether this type type can be copy-constructed with memcpy
        has_trivial_copy    = is_pointer || trait_trivial_copy<TYPE>::value,
        // whether this type can be moved with memmove
        has_trivial_move    = is_pointer || trait_trivial_move<TYPE>::value
    };
};

template <typename T, typename U>
struct aggregate_traits {
    enum {
        is_pointer          = false,
        has_trivial_ctor    = 
            traits<T>::has_trivial_ctor && traits<U>::has_trivial_ctor,
        has_trivial_dtor    = 
            traits<T>::has_trivial_dtor && traits<U>::has_trivial_dtor,
        has_trivial_copy    = 
            traits<T>::has_trivial_copy && traits<U>::has_trivial_copy,
        has_trivial_move    = 
            traits<T>::has_trivial_move && traits<U>::has_trivial_move
    };
};

#define ANDROID_BASIC_TYPES_TRAITS( T )                                     \
    template<> struct trait_trivial_ctor< T >   { enum { value = true }; }; \
    template<> struct trait_trivial_dtor< T >   { enum { value = true }; }; \
    template<> struct trait_trivial_copy< T >   { enum { value = true }; }; \
    template<> struct trait_trivial_move< T >   { enum { value = true }; };

// ---------------------------------------------------------------------------

/*
 * basic types traits
 */

ANDROID_BASIC_TYPES_TRAITS( void )
ANDROID_BASIC_TYPES_TRAITS( bool )
ANDROID_BASIC_TYPES_TRAITS( char )
ANDROID_BASIC_TYPES_TRAITS( unsigned char )
ANDROID_BASIC_TYPES_TRAITS( short )
ANDROID_BASIC_TYPES_TRAITS( unsigned short )
ANDROID_BASIC_TYPES_TRAITS( int )
ANDROID_BASIC_TYPES_TRAITS( unsigned int )
ANDROID_BASIC_TYPES_TRAITS( long )
ANDROID_BASIC_TYPES_TRAITS( unsigned long )
ANDROID_BASIC_TYPES_TRAITS( long long )
ANDROID_BASIC_TYPES_TRAITS( unsigned long long )
ANDROID_BASIC_TYPES_TRAITS( float )
ANDROID_BASIC_TYPES_TRAITS( double )

// ---------------------------------------------------------------------------


/*
 * compare and order types
 */

template<typename TYPE> inline
int strictly_order_type(const TYPE& lhs, const TYPE& rhs) {
    return (lhs < rhs) ? 1 : 0;
}

template<typename TYPE> inline
int compare_type(const TYPE& lhs, const TYPE& rhs) {
    return strictly_order_type(rhs, lhs) - strictly_order_type(lhs, rhs);
}

/*
 * create, destroy, copy and move types...
 */

template<typename TYPE> inline
void construct_type(TYPE* p, size_t n) {
    if (!traits<TYPE>::has_trivial_ctor) {
        while (n--) {
            new(p++) TYPE;
        }
    }
}

template<typename TYPE> inline
void destroy_type(TYPE* p, size_t n) {
    if (!traits<TYPE>::has_trivial_dtor) {
        while (n--) {
            p->~TYPE();
            p++;
        }
    }
}

template<typename TYPE> inline
void copy_type(TYPE* d, const TYPE* s, size_t n) {
    if (!traits<TYPE>::has_trivial_copy) {
        while (n--) {
            new(d) TYPE(*s);
            d++, s++;
        }
    } else {
        memcpy(d,s,n*sizeof(TYPE));
    }
}

template<typename TYPE> inline
void splat_type(TYPE* where, const TYPE* what, size_t n) {
    if (!traits<TYPE>::has_trivial_copy) {
        while (n--) {
            new(where) TYPE(*what);
            where++;
        }
    } else {
        while (n--) {
            *where++ = *what;
        }
    }
}

template<typename TYPE> inline
void move_forward_type(TYPE* d, const TYPE* s, size_t n = 1) {
    if ((traits<TYPE>::has_trivial_dtor && traits<TYPE>::has_trivial_copy) 
            || traits<TYPE>::has_trivial_move) 
    {
        memmove(d,s,n*sizeof(TYPE));
    } else {
        d += n;
        s += n;
        while (n--) {
            --d, --s;
            if (!traits<TYPE>::has_trivial_copy) {
                new(d) TYPE(*s);
            } else {
                *d = *s;   
            }
            if (!traits<TYPE>::has_trivial_dtor) {
                s->~TYPE();
            }
        }
    }
}

template<typename TYPE> inline
void move_backward_type(TYPE* d, const TYPE* s, size_t n = 1) {
    if ((traits<TYPE>::has_trivial_dtor && traits<TYPE>::has_trivial_copy) 
            || traits<TYPE>::has_trivial_move) 
    {
        memmove(d,s,n*sizeof(TYPE));
    } else {
        while (n--) {
            if (!traits<TYPE>::has_trivial_copy) {
                new(d) TYPE(*s);
            } else {
                *d = *s;   
            }
            if (!traits<TYPE>::has_trivial_dtor) {
                s->~TYPE();
            }
            d++, s++;
        }
    }
}


// ---------------------------------------------------------------------------

/*
 * a key/value pair
 */

template <typename KEY, typename VALUE>
struct key_value_pair_t {
    KEY     key;
    VALUE   value;
    key_value_pair_t() { }
    key_value_pair_t(const key_value_pair_t& o) : key(o.key), value(o.value) { }
    key_value_pair_t(const KEY& k, const VALUE& v) : key(k), value(v)  { }
    key_value_pair_t(const KEY& k) : key(k) { }
    inline bool operator < (const key_value_pair_t& o) const {
        return strictly_order_type(key, o.key);
    }
};

template<>
template <typename K, typename V>
struct trait_trivial_ctor< key_value_pair_t<K, V> >
{ enum { value = aggregate_traits<K,V>::has_trivial_ctor }; };
template<> 
template <typename K, typename V>
struct trait_trivial_dtor< key_value_pair_t<K, V> >
{ enum { value = aggregate_traits<K,V>::has_trivial_dtor }; };
template<> 
template <typename K, typename V>
struct trait_trivial_copy< key_value_pair_t<K, V> >
{ enum { value = aggregate_traits<K,V>::has_trivial_copy }; };
template<> 
template <typename K, typename V>
struct trait_trivial_move< key_value_pair_t<K, V> >
{ enum { value = aggregate_traits<K,V>::has_trivial_move }; };

// ---------------------------------------------------------------------------

}; // namespace android

// ---------------------------------------------------------------------------

#endif // ANDROID_TYPE_HELPERS_H
