#pragma once
#include "DxobTypes.hpp"
#include <span>

namespace Dxob
{
    // The layout looks ugly, but it's better than having 10 bytes of padding
    struct FileSettings
    {
        u32 DxobVersion;
        bool usesPerRowJumps;
        bool isGZipCompressed;
        u64 height;
        u64 width;
        u64 dataLength;
    };
}