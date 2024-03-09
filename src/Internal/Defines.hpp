#pragma once
#include <assert.h>
#include <exception>
#include <stdexcept>
#include <immintrin.h>

#ifndef DXOB_ASSERT
#define DXOB_ASSERT(x) assert(x)
#endif

#ifndef DXOB_NOTHROW
#define DXOB_RUNTIME(x) throw std::runtime_error(x) 
#else
#define DXOB_RUNTIME(x) exit(1)
#endif

#ifndef DXOB_FUNCPTR
#define DXOB_FUNCPTR(retType, name, ...) retType(*##name##)(__VA_ARGS__);
#endif

#ifndef BYTE_SIZE
#define BYTE_SIZE sizeof(u8) * 8
#endif

#ifndef BIT_SIZE
#define BIT_SIZE(x) (sizeof(x) * 8)
#endif

#ifndef CLIP_TOP_BIT
#define CLIP_TOP_BIT(T) T & ~(1 << (sizeof(decltype(T)) * 8 - 1))
#endif

#ifndef CLIP_BOTTOM_BIT
#define CLIP_BOTTOM_BIT(T) T & ~(1)
#endif

#ifndef SZOV
#define SZOV(x) (sizeof(decltype(x)))
#endif

#ifndef DISABLE_SIMD
#define DXOB_USING_SIMD
#endif