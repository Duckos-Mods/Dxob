#pragma once
#include <concepts>
#include <vector>
#include "Defines.hpp"

namespace Dxob
{
    using u8 = unsigned char;
    using u16 = unsigned short;
    using u32 = unsigned int;
    using u64 = unsigned long long;

    using i8 = signed char;
    using i16 = signed short;
    using i32 = signed int;
    using i64 = signed long long;
    
    template<typename T>
    concept DataStore = requires(T t)
    {
        { t.size() } -> std::same_as<u64>;
        { t.operator[](u64{}) };
        { t.push_back({}) } -> std::same_as<void>;
    };
    #ifndef DXOB_DATASTORE
    #define DXOB_DATASTORE std::vector
    #endif

    static_assert(DataStore<DXOB_DATASTORE<u8>>, "DXOB_DATASTORE must satisfy the DataStore concept");
}