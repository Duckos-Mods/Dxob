#pragma once
#include "DxobSettings.hpp"
#include "Defines.hpp"

typedef DXOB_FUNCPTR(Dxob::u16, PositionTranslationFunction, Dxob::i64, Dxob::i64);

namespace Dxob
{
    class HeightDataAccessor
    {
        public:
            HeightDataAccessor(std::vector<u16> heightData, FileSettings fileSettings) : m_fileSettings(fileSettings), m_heightData(heightData) {}
			HeightDataAccessor() {}



            FileSettings GetFileSettings() const { return m_fileSettings; }
            std::vector<u16> GetHeightData() const { return m_heightData; }
            FileSettings& GetFileSettings() { return m_fileSettings; }
            std::vector<u16>& GetHeightData() { return m_heightData; }



            void SetPositionTranslationFunction(PositionTranslationFunction function) { m_positionTranslationFunction = function; }
        private:
            PositionTranslationFunction m_positionTranslationFunction = nullptr;

            FileSettings m_fileSettings;
            std::vector<u16> m_heightData;
    };
}