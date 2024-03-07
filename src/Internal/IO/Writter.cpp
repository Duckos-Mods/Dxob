#include "Writter.hpp"
#include "../DxobByteArrayWrapper.hpp"
#include <tuple>
namespace Dxob
{
	void Writer::Write(const HeightDataAccessor& data, std::ostream& writeLoc)
	{
		std::pair<u16, u64> t;
	}
	void Writer::WriteHeader(const HeightDataAccessor& data, std::ostream& writeLoc, bool gzipData)
	{
		u64 fileStart = 'DXOB';
		writeLoc.write(reinterpret_cast<char*>(&fileStart), sizeof(u64));
		FileSettings fileSettings = data.GetFileSettings();
		writeLoc.write(reinterpret_cast<char*>(&fileSettings.DxobVersion), sizeof(u32));
		// liitle endian u8
		u8 boolData = 0x0;
		if (gzipData)
			boolData = 0b00000001;

		auto isBetter = IsPerRowBetter(data, writeLoc);
		if (isBetter.isBetter)
			boolData |= 0b00000010;
		writeLoc.write(reinterpret_cast<char*>(&boolData), sizeof(u8));
		if (isBetter.isBetter)
			WritePerRowDeltas(isBetter.deltas, writeLoc, isBetter);
		else
			WriteAllData(data, writeLoc, isBetter);
	}

	void Writer::WritePerRowDeltas(const std::vector<u16>& deltas, std::ostream& writeLoc, dataCollectionReturn& d)
	{

	}

	void Writer::WriteAllData(const HeightDataAccessor& data, std::ostream& writeLoc, dataCollectionReturn& d)
	{
		u64 minBits = CalculateMinBitsForValue(d.bitCost);
		u64 totalDataLeng = data.GetFileSettings().height * data.GetFileSettings().width;
		u64 bytecount = CalculateBytesForBitsPerValue(minBits, totalDataLeng);
		ByteArrayWrapper<u16> dataWrapper(bytecount, minBits);
		std::span<u16> heightData(data.GetHeightData().data(), data.GetHeightData().size());
		for (u64 i = 0; i < heightData.size(); i++)
			dataWrapper.NoGuardWrite(i, heightData[i]);
		writeLoc.write(reinterpret_cast<char*>(&bytecount), sizeof(u64));
		writeLoc.write(reinterpret_cast<char*>(dataWrapper.GetDataRaw()), bytecount);
	}

	u64 Writer::CalculateBytesForBitsPerValue(u64 bits, u64 count)
	{
		u64 bytes = bits * count / 8;
		while (bytes % 8 != 0)
			bytes++;
		return bytes;
	}

	u16 Writer::CalculateMaxDelta(std::span<u16> data)
	{
		u16 maxDelta = 0;
		for (u64 i = 0; i < data.size() - 2; i++)
		{
			u16 delta = data[i + 1] - data[i];
			if (delta > maxDelta)
				maxDelta = delta;
		}
		return maxDelta;
	}
	Writer::dataCollectionReturn Writer::IsPerRowBetter(const HeightDataAccessor& data, std::ostream& writeLoc)
	{
		std::span<u16> heightData(data.GetHeightData().data(), data.GetHeightData().size());
		u16 maxDeltaForAllData = CalculateMaxDelta(heightData);
		u64 maxDeltaForAllDataBitCost = sizeof(u16) * heightData.size() - 1;


		u64 maxDeltaForPerRowBitCost = sizeof(u16) * data.GetFileSettings().height; // Calculate the size of the array of deltas
		std::vector<u16> perRowsDeltas(data.GetFileSettings().height, 0); // Create a vector to store the deltas for each row
		for (u64 i = 0; i < data.GetFileSettings().height; i++)
		{
			u16 maxDelta = CalculateMaxDelta(heightData.subspan(i * data.GetFileSettings().width, data.GetFileSettings().width));
			u16 maxBits = CalculateMinBitsForValue(maxDelta);
			if (maxBits >= 0)
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