//
//  BRInt.h
//
//  Created by Aaron Voisine on 8/16/15.
//  Copyright (c) 2015 breadwallet LLC.
//  Copyright (c) 2018 elastos zxb.
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  THE SOFTWARE.

#ifndef BRInt_h
#define BRInt_h

#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

// large integers
union _u16 {
    uint8_t u8[16/8];
};

union _u32 {
    uint8_t u8[32/8];
};

union _u64 {
    uint8_t u8[64/8];
};

typedef union _u128 {
    uint8_t u8[128/8];
    uint16_t u16[128/16];
    uint32_t u32[128/32];
    uint64_t u64[128/64];
} UInt128;

typedef union _u160 {
    uint8_t u8[160/8];
    uint16_t u16[160/16];
    uint32_t u32[160/32];
} UInt160;

typedef union _u168 {
    uint8_t u8[168/8];
}UInt168;

typedef union _u256 {
    uint8_t u8[256/8];
    uint16_t u16[256/16];
    uint32_t u32[256/32];
    uint64_t u64[256/64];
} UInt256;

typedef union _u512 {
    uint8_t u8[512/8];
    uint16_t u16[512/16];
    uint32_t u32[512/32];
    uint64_t u64[512/64];
} UInt512;

int UInt128Eq(const UInt128* a, const UInt128* b);

int UInt160Eq(const UInt160* a, const UInt160* b);

int UInt168Eq(const UInt168* a, const UInt168* b);

int UInt256Eq(const UInt256* a, const UInt256* b);

int UInt256LessThan(const UInt256* a, const UInt256* b);

int UInt512Eq(const UInt512* a, const UInt512* b);

int UInt128IsZero(const UInt128* u);

int UInt160IsZero(const UInt160* u);

int UInt168IsZero(const UInt168* u);

int UInt256IsZero(const UInt256* u);

int UInt512IsZero(const UInt512* u);

UInt256 UInt256Reverse(const UInt256* u);

#define UINT128_ZERO ((UInt128) { .u64 = { 0, 0 } })
#define UINT160_ZERO ((UInt160) { .u32 = { 0, 0, 0, 0, 0 } })
#define UINT168_ZERO ((UInt168) { .u8 = { 0, 0, 0, 0, 0 ,0 , 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0} })
#define UINT256_ZERO ((UInt256) { .u64 = { 0, 0, 0, 0 } })
#define UINT512_ZERO ((UInt512) { .u64 = { 0, 0, 0, 0, 0, 0, 0, 0 } })

#define _hexc(u) (((u) & 0x0f) + ((((u) & 0x0f) <= 9) ? '0' : 'a' - 0x0a))
#define _hexu(c) (((c) >= '0' && (c) <= '9') ? (c) - '0' : ((c) >= 'a' && (c) <= 'f') ? (c) - ('a' - 0x0a) :\
                  ((c) >= 'A' && (c) <= 'F') ? (c) - ('A' - 0x0a) : -1)

// unaligned memory access helpers

void UInt8SetBE(void *b2, uint16_t u);

void UInt8SetLE(void *b2, uint16_t u);

void UInt16SetBE(void *b2, uint16_t u);

void UInt16SetLE(void *b2, uint16_t u);

void UInt32SetBE(void *b4, uint32_t u);

void UInt32SetLE(void *b4, uint32_t u);

void UInt64SetBE(void *b8, uint64_t u);

void UInt64SetLE(void *b8, uint64_t u);

void UInt128Set(void *b16, UInt128 u);

void UInt160Set(void *b20, UInt160 u);

void UInt168Set(void *b21, UInt168 u);

void UInt256Set(void *b32, UInt256 u);

uint8_t UInt8GetBE(const void *b2);

uint8_t UInt8GetLE(const void *b2);

uint16_t UInt16GetBE(const void *b2);

uint16_t UInt16GetLE(const void *b2);

uint32_t UInt32GetBE(const void *b4);

uint32_t UInt32GetLE(const void *b4);

uint64_t UInt64GetBE(const void *b8);

uint64_t UInt64GetLE(const void *b8);

void UInt128Get(UInt128* value, const void *b16);

void UInt160Get(UInt160* value, const void *b20);

void UInt168Get(UInt168* value, const void *b21);

void UInt256Get(UInt256* value, const void *b32);

#ifdef __cplusplus
}
#endif

#endif // BRInt_h
