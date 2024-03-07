#pragma once
#include "../DxobTypes.hpp"
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
		void Write(const HeightDataAccessor& data, std::ostream& writeLoc);
	private:
		struct dataCollectionReturn
		{
			bool isBetter;
			u64 bitCost;
			std::vector<u16> deltas;
		};
	private:
		void WriteHeader(const HeightDataAccessor& data, std::ostream& writeLoc, bool gzipData = true);
		u16 CalculateMaxDelta(std::span<u16> data);
		dataCollectionReturn IsPerRowBetter(const HeightDataAccessor& data, std::ostream& writeLoc);
		u8 CalculateMinBitsForValue(u16 value);

		void WritePerRowDeltas(const std::vector<u16>& deltas, std::ostream& writeLoc, dataCollectionReturn& d);
		void WriteAllData(const HeightDataAccessor& data, std::ostream& writeLoc, dataCollectionReturn& d);
		u64 CalculateBytesForBitsPerValue(u64 bits, u64 count);
	};
}