#pragma once
#include "../DxobTypes.hpp"
#include "../DxobMemory.hpp"
#include "../DxobSettings.hpp"
#include "../DxobData.hpp"
#include "../Defines.hpp"
#include <span>
#include <memory>
#include <fstream>
namespace Dxob
{


    class Reader
    {
    public:
        Reader() {}
        bool IsDxob(BinaryStream& data);
        HeightDataAccessor Process(BinaryStream& data);
        HeightDataAccessor Process(std::string_view path);
    private:
        struct UsedData{
            u64 height;
            u64 width;
            u64 size = 0;
            u8 bitsPerIndex = 0;
        };
    private:
        void ReadPerRowDeltas(BinaryStream& data, UsedData& usedData, std::vector<u16>& heightData);
        void ReadAllData(BinaryStream& data, UsedData& usedData, u8** writeLoc);
    };


}