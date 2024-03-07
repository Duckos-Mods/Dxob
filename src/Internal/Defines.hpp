#pragma once
#include <assert.h>
#include <exception>
#include <stdexcept>

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