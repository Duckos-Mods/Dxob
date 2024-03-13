#include "Writter.hpp"
#include "../DxobByteArrayWrapper.hpp"
#include <tuple>
#include <algorithm>
#include <ranges>
#include "../Defines.hpp"
#include <limits>

namespace Dxob
{
	static const char* eoa = "EOA";
	void Writer::Write(HeightDataAccessor& data, BinaryStream& writeLoc)
	{
		u32 fileStart = 'DXOB';
		FileSettings fileSettings = data.GetFileSettings();
		// liitle endian u8
		u8 boolData = 0x0;
		
		//if (data.IsGzip()) // Removed GZIP becuase due to bitpacking, it can make the data larger than it was before
		//	boolData = 0b00000001;
		data.SetGzip(false);

		auto isBetter = IsPerRowBetter(data);
		if (isBetter.isBetter)
			boolData |= 0b00000010;
		writeLoc.write(&fileStart)
			.write(&fileSettings.DxobVersion)
			.write(&boolData)
			.write(&fileSettings.height)
			.write(&fileSettings.width);
		BinaryStream dataWriteLocStream(8192);
		if (isBetter.isBetter)
			WritePerRowDeltas(isBetter.deltas, dataWriteLocStream, data);
		else
			WriteAllData(data, dataWriteLocStream);

		writeLoc.write(dataWriteLocStream.data(), dataWriteLocStream.size());              

		return; 
	}

	void Writer::WritePerRowDeltas(const std::vector<u16>& deltas, BinaryStream& writeLoc, HeightDataAccessor& data)
	{
		/*auto& dt = d.deltas;
		u64 size = dt.size();
		writeLoc.write(&size)
			.write(std::span<u16>(dt.data(), dt.size()))
			.write(eoa, 3);*/
		const u64 with = data.GetFileSettings().width;
		std::span<u16> heightData(data.GetHeightData().data(), data.GetHeightData().size());
		for (int i = 0; i < deltas.size(); i++)
		{
			u16 delta = deltas[i];
			u8 bits = CalculateMinBitsForValue(delta);
			std::span<u16> row = heightData.subspan(i * with, with);
			u16 lowest = std::min_element(row.begin(), row.end()).operator*();
			ByteArrayWrapper<u16> dataWrapper(row.size(), bits);
			u64 sliceSize = dataWrapper.GetTotalBytesTaken();
			for (u64 x = 0; x < row.size(); x++)
				dataWrapper.NoGuardWrite(x, row[x] - lowest);
			writeLoc.write(&lowest)
				.write(&bits)
				.write(&sliceSize)
				.write(std::span<u8>(dataWrapper.GetDataRaw(), dataWrapper.GetTotalBytesTaken()))
				.write(eoa, 3);
		}
	}

	void Writer::WriteAllData(HeightDataAccessor& data, BinaryStream& writeLoc)
	{
		std::span<u16> heightData(data.GetHeightData().data(), data.GetHeightData().size());
		u16 maxDelta = CalculateMaxDelta(heightData);
		u8 bits = CalculateMinBitsForValue(maxDelta);
		u16 lowest = std::min_element(heightData.begin(), heightData.end()).operator*();
		u64 size = CalculateBytesForBitsPerValue(bits, heightData.size());
		ByteArrayWrapper<u16> dataWrapper(heightData.size(), bits);
		for (u64 i = 0; i < heightData.size(); i++)
			dataWrapper.NoGuardWrite(i, heightData[i]);
		writeLoc.write(&lowest)
			.write(&bits)
			.write(&size)
			.write(std::span<u8>(dataWrapper.GetDataRaw(), dataWrapper.GetTotalBytesTaken()))
			.write(eoa, 3);
	}

	u64 Writer::CalculateBytesForBitsPerValue(u64 bits, u64 count)
	{
		u64 bytes = bits * count / 8;
		while (bytes % 8 != 0)
			bytes++;
		return bytes;
	}
#ifndef DXOB_USING_SIMD
	u16 Writer::CalculateMaxDelta(std::span<u16> data)
	{
		uint16_t minNum = std::numeric_limits<uint16_t>::max();
		uint16_t maxNum = std::numeric_limits<uint16_t>::min();

		for (const auto& num : data) {
			if (num < minNum) {
				minNum = num;
			}
			if (num > maxNum) {
				maxNum = num;
			}
		}

		return maxNum - minNum;
	}
#else
	u16 Writer::CalculateMaxDelta(std::span<u16> data)
	{
		uint16_t minNum = std::numeric_limits<uint16_t>::max();
		uint16_t maxNum = std::numeric_limits<uint16_t>::min();

		const size_t chunkSize = 256 / BIT_SIZE(u16); // 256-bit AVX registers hold 16 uint16_t values
		const size_t numChunks = data.size() / chunkSize;

		// Iterate over chunks
		for (size_t i = 0; i < numChunks; ++i) {
			__m256i chunk = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&data[i * chunkSize]));

			__m256i chunkMin = _mm256_min_epu16(chunk, _mm256_shuffle_epi32(chunk, _MM_SHUFFLE(0, 0, 3, 2)));
			__m256i chunkMax = _mm256_max_epu16(chunk, _mm256_shuffle_epi32(chunk, _MM_SHUFFLE(0, 0, 3, 2)));

			chunkMin = _mm256_min_epu16(chunkMin, _mm256_shuffle_epi32(chunkMin, _MM_SHUFFLE(0, 0, 0, 1)));
			chunkMax = _mm256_max_epu16(chunkMax, _mm256_shuffle_epi32(chunkMax, _MM_SHUFFLE(0, 0, 0, 1)));

			uint16_t minArr[16] = { 0 };
			uint16_t maxArr[16] = { 0 };
			_mm256_storeu_si256(reinterpret_cast<__m256i*>(minArr), chunkMin);
			_mm256_storeu_si256(reinterpret_cast<__m256i*>(maxArr), chunkMax);

			for (size_t j = 0; j < 16; ++j) {
				if (minArr[j] < minNum) 
					minNum = minArr[j];
				if (maxArr[j] > maxNum) 
					maxNum = maxArr[j];
			}
		}

		// Handle the remaining elements
		for (size_t i = numChunks * chunkSize; i < data.size(); ++i) {
			if (data[i] < minNum) 
				minNum = data[i];
			if (data[i] > maxNum) 
				maxNum = data[i];
		}
		return maxNum - minNum;
	}
#endif




	Writer::dataCollectionReturn Writer::IsPerRowBetter(HeightDataAccessor& data)
	{
		std::span<u16> heightData(data.GetHeightData().data(), data.GetHeightData().size());
		u16 maxDeltaForAllData = CalculateMaxDelta(heightData);
		u8 maxBitsForAllData = CalculateMinBitsForValue(maxDeltaForAllData);
		u64 maxDeltaForAllDataBitCost = maxBitsForAllData * heightData.size();


		u64 maxDeltaForPerRowBitCost = 0; // Calculate the size of the array of deltas
		u64 height = data.GetFileSettings().height;
		std::vector<u16> perRowsDeltas(height, 0); // Create a vector to store the deltas for each row
		for (u64 i = 0; i < height; i++)
		{
			u16 maxDelta = CalculateMaxDelta(heightData.subspan(i * data.GetFileSettings().width, data.GetFileSettings().width));
			u8 maxBits = CalculateMinBitsForValue(maxDelta);
			if (maxBits <= 0)
				maxBits = 1; // If the maxBits is 0, set it to 1 to avoid a divide by 0 error
			perRowsDeltas[i] = maxDelta;
			maxDeltaForPerRowBitCost += maxBits * data.GetFileSettings().width;
		}

		if (maxDeltaForAllDataBitCost < maxDeltaForPerRowBitCost)
			return { false, maxDeltaForAllData, {} };
		else
			return { true, maxDeltaForPerRowBitCost, perRowsDeltas };

	}
	u8 Writer::CalculateMinBitsForValue(u16 value)
	{
		u8 bits = 0;
		while (value > 0)
		{
			value >>= 1;
			bits++;
		}
		return bits;
	}
}