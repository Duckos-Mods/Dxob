#include "DxobData.hpp"

namespace Dxob
{
	u16 HeightDataAccessor::HeightAt(i64 x, i64 y) const
	{
		if (m_positionTranslationFunction == nullptr)
			return m_heightData[y * m_fileSettings.width + x];
		else [[unlikely]]
			return m_heightData[m_positionTranslationFunction(x, y)];
	}
	void HeightDataAccessor::SetHeightAt(i64 x, i64 y, u16 value)
	{
		if (m_positionTranslationFunction != nullptr) {
			m_heightData[m_positionTranslationFunction(x, y)] = value;
		}
		else [[likely]] {
			m_heightData[y * m_fileSettings.width + x] = value;
		}
	}
}