#include "Reader.hpp"
#include "../DxobByteArrayWrapper.hpp"
#include <zlib.h>

namespace Dxob
{
	static u32 expectedFileStart = 'DXOB';
	static u32 expectedFileVersion = '0001';
	bool Reader::IsDxob(BinaryStream& data)
	{
		if (data.size() < 4)
			return false;
		u32 fileStart = 0;
		data.peakn(&fileStart, SZOV(fileStart));
		return fileStart == expectedFileStart;
	}
	HeightDataAccessor Reader::Process(BinaryStream& data)
	{
		if (!IsDxob(data))
			DXOB_RUNTIME("Not a DXOB file. Use Reader::IsDxob(data) to check if the file is a valid dxob format");
		FileSettings fileSettings;
		data.seekg(4);
		u32 fileVersion = 0;
		data.read(&fileVersion);
		if (fileVersion != expectedFileVersion)
			DXOB_RUNTIME("File version is not supported");
		u8 boolData = 0;
		data.read(&boolData);
		bool isGzip = boolData & 0b00000001;
		bool isPerRow = boolData & 0b00000010;
		u64 height = 0;
		u64 width = 0;
		data.read(&height)
			.read(&width);

		fileSettings.width = width;
		fileSettings.height = height;
		fileSettings.isGZipCompressed = isGzip;
		fileSettings.usesPerRowJumps = isPerRow;
		HeightDataAccessor hda;
		hda.SetFileSettings(fileSettings);
		UsedData usedData(height, width);

		BinaryStream& parseBuffer = data; 
		if (isGzip) [[unlikely]]
		{
			uLongf uncompressedSize = 0;
			uLongf compressedSize = uLongf(data.size() - data.tellg());
			u8* compressedData = talloc<u8>(compressedSize);
			if (compressedData == nullptr)
				DXOB_THROW(std::bad_alloc());
			data.read(compressedData, compressedSize);
			uncompressedSize = compressBound(compressedSize);
			u8* uncompressedData = talloc<u8>(uncompressedSize);
			if (uncompressedData == nullptr)
				DXOB_THROW(std::bad_alloc());
			uncompress(uncompressedData, &uncompressedSize, compressedData, compressedSize);
			parseBuffer = BinaryStream(uncompressedData, u32(uncompressedSize)); // Set the parse buffer to the uncompressed data
		}
		
		if (isPerRow)
		{
			std::vector<u16> heightData;
			ReadPerRowDeltas(parseBuffer, usedData, heightData);
			hda.SetHeightData(heightData);
		}
		else [[unlikely]]
		{
			u8* writeData = nullptr;
			ReadAllData(parseBuffer, usedData, &writeData);
			ByteArrayWrapper<u16> rawData;
			rawData.SetUnderlyingData(writeData, usedData.size, usedData.bitsPerIndex);
			std::vector<u16> heightData;
			heightData.resize(usedData.height * usedData.width);
			for (u64 i = 0; i < usedData.height; i++)
				for (u64 j = 0; j < usedData.width; j++)
					heightData[i * usedData.width + j] = rawData[i * usedData.width + j];
			hda.SetHeightData(heightData);
		}
		return hda;
	}

	HeightDataAccessor Reader::Process(std::string_view path)
	{
		BinaryStream data;
		std::ifstream file(path.data(), std::ios::binary);
		if (!file.is_open())
			DXOB_RUNTIME("Failed to open file");
		file.seekg(0, std::ios::end);
		auto size = file.tellg();
		file.seekg(0, std::ios::beg);
		data.resize(size);
		file.read(reinterpret_cast<char*>(data.data()), size);
		file.close();
		return Process(data);
	}

	void Reader::ReadPerRowDeltas(BinaryStream& data, UsedData& usedData, std::vector<u16>& heightData)
	{
		char eoaBuffer[3];
		auto EOAChecker = [&]() -> void {
			if (eoaBuffer[0] != 'E' || eoaBuffer[1] != 'O' || eoaBuffer[2] != 'A')
				DXOB_RUNTIME("End of array marker not found");
			};
		heightData.resize(usedData.height * usedData.width);
		// std::vector<u16> deltaArray(usedData.height);
		/*data.read(&deltaArraySize)
			.read(deltaArray.data(), deltaArraySize*sizeof(u16))
		.read(eoaBuffer, 3);
		EOAChecker();*/
		for (u64 i = 0; i < usedData.height; i++)
		{
			u16 lowest = 1;
			u8 bits = 0;
			u64 rowByteSize = 0;
			data.read(&lowest)
				.read(&bits)
				.read(&rowByteSize);
			u8* rowBytes = talloc<u8>(rowByteSize);
			if (rowBytes == nullptr)
				DXOB_THROW(std::bad_alloc());
			data.read(rowBytes, rowByteSize)
				.read(eoaBuffer, 3);
			EOAChecker();
			ByteArrayWrapper<u16> rawData;
			rawData.SetUnderlyingData(rowBytes, usedData.width, bits);
			for (u64 j = 0; j < usedData.width; j++)
				heightData[i * usedData.width + j] = lowest + rawData[j];
			// We DO NOT free rowBytes because the ByteArrayWrapper takes ownership of the memory and will free it when it goes out of scope
		}
	}

	void Reader::ReadAllData(BinaryStream& data, UsedData& usedData, u8** writeLoc)
	{
		u16 lowest = 0;
		u8 bits = 0;
		u64 size = 0;
		data.read(&lowest)
			.read(&bits)
			.read(&size);
		u64 bytesToRead = size;
		u8 eoaBuffer[3];
		u8* wl = talloc<u8>(bytesToRead);

		if (wl == nullptr)
			DXOB_RUNTIME("Failed to allocate memory");

		if (bytesToRead % 8 != 0)
			DXOB_RUNTIME("bytesToRead is not a multiple of 8 something has gone wrong");

		data.read(wl, bytesToRead)
			.read(eoaBuffer, 3);

		if (eoaBuffer[0] != 'E' || eoaBuffer[1] != 'O' || eoaBuffer[2] != 'A')
			DXOB_RUNTIME("End of array marker not found");
		usedData.size = size;
		usedData.bitsPerIndex = bits;
		*writeLoc = wl; // Set the write location to the allocated memory



	}

}