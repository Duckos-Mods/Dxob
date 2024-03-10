#include "Writter.hpp"
#include "../DxobByteArrayWrapper.hpp"
#include <tuple>
#include <zlib.h>
#include <algorithm>
#include <ranges>
#include <limits>

namespace Dxob
{
	void Writer::Write(HeightDataAccessor& data, BinaryStream& writeLoc)
	{
		u32 fileStart = 'DXOB';
		FileSettings fileSettings = data.GetFileSettings();
		// liitle endian u8
		u8 boolData = 0x0;
		if (data.IsGzip())
			boolData = 0b00000001;

		auto isBetter = IsPerRowBetter(data, writeLoc);
		if (isBetter.isBetter)
			boolData |= 0b00000010;
		writeLoc.write(reinterpret_cast<u8*>(&fileStart), SZOV(fileStart))
			.write(reinterpret_cast<u8*>(&fileSettings.DxobVersion), sizeof(u32))
			.write(reinterpret_cast<u8*>(&boolData), sizeof(u8));
		BinaryStream dataWriteLocStream(8192);

		if (isBetter.isBetter)
			WritePerRowDeltas(isBetter.deltas, dataWriteLocStream, isBetter, data);
		else
			WriteAllData(data, dataWriteLocStream, isBetter);

		if (data.IsGzip())
		{
			// Create a buffer to store the compressed data
			std::vector<u8> compressedData;
			compressedData.resize(compressBound(dataWriteLocStream.size()));
			// Compress the data
			uLongf compressedDataSize = compressedData.size();
			compress(compressedData.data(), &compressedDataSize, reinterpret_cast<const Bytef*>(dataWriteLocStream.data()), dataWriteLocStream.size());
			// Write the compressed data to the file
			
			writeLoc.write(reinterpret_cast<u8*>(compressedData.data()), compressedDataSize);
		}
		else
			writeLoc.write(dataWriteLocStream.data(), dataWriteLocStream.size());

		return; 
	}

	void Writer::WritePerRowDeltas(const std::vector<u16>& deltas, BinaryStream& writeLoc, dataCollectionReturn& d, HeightDataAccessor& data)
	{
		auto& dt = d.deltas;
		u64 size = dt.size();
		const char* eoa = "EOA";
		writeLoc.write(reinterpret_cast<u8*>(&size), sizeof(u64))
			.write(std::span<u16>(dt.data(), dt.size()))
			.write(reinterpret_cast<u8*>(const_cast<char*>(eoa)), 3);
		const u64 with = data.GetFileSettings().width;
		std::span<u16> heightData(data.GetHeightData().data(), data.GetHeightData().size());
		for (int i = 0; i < deltas.size(); i++)
		{
			u16 delta = deltas[i];
			u8 bits = CalculateMinBitsForValue(delta);
			std::span<u16> row = heightData.subspan(i * with, with);
			u16 lowest = std::min_element(row.begin(), row.end()).operator*();
			ByteArrayWrapper<u16> dataWrapper(row.size(), bits);
			u64 size = dataWrapper.GetTotalBytesTaken();
			for (u64 i = 0; i < row.size(); i++)
				dataWrapper.NoGuardWrite(i, row[i] - lowest);
			writeLoc.write(reinterpret_cast<u8*>(&lowest), sizeof(u16))
				.write(reinterpret_cast<u8*>(&bits), sizeof(u8))
				.write(reinterpret_cast<u8*>(&size), sizeof(u64))
				.write(std::span<u8>(dataWrapper.GetDataRaw(), dataWrapper.GetTotalBytesTaken()))
				.write(reinterpret_cast<u8*>(const_cast<char*>(eoa)), 3);
		}
	}

	void Writer::WriteAllData(HeightDataAccessor& data, BinaryStream& writeLoc, dataCollectionReturn& d)
	{
		u8 minBits = CalculateMinBitsForValue(d.bitCost);
		u64 totalDataLeng = data.GetFileSettings().height * data.GetFileSettings().width;
		u64 bytecount = CalculateBytesForBitsPerValue(minBits, totalDataLeng);
		ByteArrayWrapper<u16> dataWrapper(bytecount, minBits);
		std::span<u16> heightData(data.GetHeightData().data(), data.GetHeightData().size());
		u16 lowest = std::min_element(heightData.begin(), heightData.end()).operator*(); 
		for (u64 i = 0; i < heightData.size(); i++)
			dataWrapper.NoGuardWrite(i, heightData[i] - lowest);
		writeLoc.write(reinterpret_cast<u8*>(&lowest), sizeof(u16))
			.write(reinterpret_cast<u8*>(&minBits), sizeof(u8))
			.write(reinterpret_cast<u8*>(&bytecount), sizeof(u64))
			.write(dataWrapper.GetDataRaw(), dataWrapper.GetTotalBytesTaken());
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

			uint16_t minArr[16];
			uint16_t maxArr[16];
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




	Writer::dataCollectionReturn Writer::IsPerRowBetter(HeightDataAccessor& data, BinaryStream& writeLoc)
	{
		std::span<u16> heightData(data.GetHeightData().data(), data.GetHeightData().size());
		u16 maxDeltaForAllData = CalculateMaxDelta(heightData);
		u8 maxBitsForAllData = CalculateMinBitsForValue(maxDeltaForAllData);
		u64 maxDeltaForAllDataBitCost = maxBitsForAllData * heightData.size();


		u64 maxDeltaForPerRowBitCost = BIT_SIZE(u8) * data.GetFileSettings().height; // Calculate the size of the array of deltas
		u64 height = data.GetFileSettings().height;
		std::vector<u16> perRowsDeltas(height, 0); // Create a vector to store the deltas for each row
		for (u64 i = 0; i < height; i++)
		{
			u8 maxDelta = CalculateMaxDelta(heightData.subspan(i * data.GetFileSettings().width, data.GetFileSettings().width));
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