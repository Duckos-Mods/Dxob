#pragma once
#include "DxobSettings.hpp"
#include "Defines.hpp"

typedef DXOB_FUNCPTR(Dxob::u16, PositionTranslationFunction, Dxob::i64, Dxob::i64);

namespace Dxob
{
    class HeightDataAccessor
    {
        public:
            HeightDataAccessor(const std::vector<u16>& heightData, FileSettings fileSettings) : m_fileSettings(fileSettings), m_heightData(heightData) {}
			HeightDataAccessor() {}



            FileSettings GetFileSettings() const { return m_fileSettings; }
            //std::vector<u16> GetHeightData() const { return m_heightData; }
            FileSettings& GetFileSettings() { return m_fileSettings; }
            std::vector<u16>& GetHeightData() { return m_heightData; }
            std::vector<u16>* GetHeightDataPtr() { return &m_heightData; }
            bool IsGzip() const { return m_fileSettings.isGZipCompressed; }
            void SetGzip(bool gzip) { m_fileSettings.isGZipCompressed = gzip; }
            void SetPositionTranslationFunction(PositionTranslationFunction function) { m_positionTranslationFunction = function; }
            void SetFileSettings(FileSettings settings) { m_fileSettings = settings; }
            u16 HeightAt(i64 x, i64 y) const;

            void SetHeightAt(i64 x, i64 y, u16 value);
        private:
            void SetHeightData(std::vector<u16>& data) { m_heightData = std::move(data); }
            friend class Reader;
        private:
            PositionTranslationFunction m_positionTranslationFunction = nullptr;

            FileSettings m_fileSettings;
            std::vector<u16> m_heightData;
    };
}