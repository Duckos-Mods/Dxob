#pragma once
#include "DxobTypes.hpp"
#include <span>

namespace Dxob
{
    // The layout looks ugly, but it's better than having 10 bytes of padding
    struct FileSettings
    {
        u32 DxobVersion = '0001'; // should be constant i dont think im going to change it
        bool usesPerRowJumps = false;
        bool isGZipCompressed = false;
        u64 height = 0;
        u64 width = 0;
    };
}