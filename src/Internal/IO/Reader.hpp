#pragma once
#include "../DxobTypes.hpp"
#include "../DxobSettings.hpp"
#include "../DxobData.hpp"
#include "../Defines.hpp"
#include <span>
#include <memory>

namespace Dxob
{


    class Reader
    {
    public:
        Reader(std::span<u8> rawData) : m_rawData(rawData) {}
        Reader(u8* rawData, size_t size) : m_rawData(rawData, size) {}
        HeightDataAccessor Process();
    private:
        std::vector<u16> ReadPerRowDeltas();
        std::span<u8> m_rawData;
    };


}