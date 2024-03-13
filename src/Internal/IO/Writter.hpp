#pragma once
#include "../DxobTypes.hpp"
#include "../DxobMemory.hpp"
#include "../DxobSettings.hpp"
#include "../DxobData.hpp"
#include "../Defines.hpp"
#include <span>
#include <iostream>
#include <string_view>

namespace Dxob
{
	class Writer
	{
	public:
		Writer() {}
		void Write(HeightDataAccessor& data, BinaryStream& writeLoc);
		static u16 CalculateMaxDelta(std::span<u16> data);
	private:
		struct dataCollectionReturn
		{
			bool isBetter;
			u64 bitCost;
			std::vector<u16> deltas;
		};
	private:
		dataCollectionReturn IsPerRowBetter(HeightDataAccessor& data);
		u8 CalculateMinBitsForValue(u16 value);

		void WritePerRowDeltas(const std::vector<u16>& deltas, BinaryStream& writeLoc, HeightDataAccessor& data);
		void WriteAllData(HeightDataAccessor& data, BinaryStream& writeLoc);
		u64 CalculateBytesForBitsPerValue(u64 bits, u64 count);
	};
}