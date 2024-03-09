#include <iostream>
#include "Dxob.hpp"
#include <random>
#include <vector>
#include <span>
#include <functional>
#include <string>
#include <fstream>
#include <chrono>
using namespace Dxob;

#define RANDOM_INT_FUNC(min, max, type) std::uniform_int_distribution<type> dis(min, max); std::mt19937 gen(std::random_device{}())

#define MULTI_TEST(func, count) for (u64 i = 0; i < count; i++) if (!func()) failed++

bool ReadAndWriteStandardBitSize()
{
	RANDOM_INT_FUNC(0, 0xFF, u16);
	ByteArrayWrapper<u16> data(4096, 8);
	std::vector<u16> dataVec;
	for (u64 i = 0; i < 4096; i++)
	{
		u16 val = dis(gen);
		dataVec.push_back(val);
	}
	for (u64 i = 0; i < 4096; i++)
		data.NoGuardWrite(i, dataVec[i]);

	for (u64 i = 0; i < 4096; i++)
	{
		if (data[i] != dataVec[i])
		{
			std::cout << "Error: " << i << " " << data[i] << " " << dataVec[i] << std::endl;
			return false;
		}
	}
	return true;
}
bool ReadAndWriteSmallerStandardBitSize()
{
	RANDOM_INT_FUNC(0, 60, u16);
	ByteArrayWrapper<u16> data(4096, 6);
	std::vector<u16> dataVec;
	for (u64 i = 0; i < 4096; i++)
	{
		u16 val = dis(gen);
		dataVec.push_back(val);
	}
	for (u64 i = 0; i < 4096; i++)
		data.NoGuardWrite(i, dataVec[i]);

	for (u64 i = 0; i < 4096; i++)
	{
		if (data[i] != dataVec[i])
		{
			std::cout << "Error: " << i << " " << data[i] << " " << dataVec[i] << std::endl;
			return false;
		}
	}
	return true;

}
bool ReadAndWriteLargerStandardBitSize()
{
	RANDOM_INT_FUNC(0, 65535, u32);
	ByteArrayWrapper<u32> data(4096, 18);
	std::vector<u32> dataVec;
	for (u64 i = 0; i < 4096; i++)
	{
		u32 val = dis(gen);
		dataVec.push_back(val);
	}
	for (u64 i = 0; i < 4096; i++)
		data.NoGuardWrite(i, dataVec[i]);

	for (u64 i = 0; i < 4096; i++)
	{
		if (data[i] != dataVec[i])
		{
			std::cout << "Error: " << i << " " << data[i] << " " << dataVec[i] << std::endl;
			return false;
		}
	}
	return true;

}
bool ReadAndWriteStandardBitSizeSigned()
{
	RANDOM_INT_FUNC(-96, 96, i16);
	ByteArrayWrapper<i8> data(4096, 8);
	std::vector<i8> dataVec;
	for (u64 i = 0; i < 4096; i++)
	{
		i8 val = i8(dis(gen));
		dataVec.push_back(val);
	}
	for (u64 i = 0; i < 4096; i++)
		data.NoGuardWrite(i, dataVec[i]);

	for (u64 i = 0; i < 4096; i++)
	{
		if (data[i] != dataVec[i])
		{
			std::cout << "Error: " << i << " " << data[i] << " " << dataVec[i] << std::endl;
			return false;
		}
	}
	return true;
}

void BenchmarkFindMaximumDelta()
{
	u64 WIDTHH = 512 * 512;
	std::vector<u16> data(WIDTHH);
	RANDOM_INT_FUNC(0, -1, u16);
	for (u64 i = 0; i < WIDTHH; i++)
		data[i] = dis(gen);
	Writer writer;
	auto start = std::chrono::high_resolution_clock::now();
	auto i = writer.CalculateMaxDelta(std::span<u16>(data));
	auto end = std::chrono::high_resolution_clock::now();
	std::cout << "Time taken: " << std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() << "us" << std::endl;
	std::cout << "Time Taken MS: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms" << std::endl;
	std::cout << "Max delta: " << i << std::endl;
}

void TestWriterAndConstructor()
{
	FileSettings settings;
	settings.isGZipCompressed = false;
#define WIDTH 6000
	settings.width = WIDTH;
	settings.height = WIDTH;
	HeightDataAccessor data(std::vector<u16>(WIDTH *WIDTH), settings);
	RANDOM_INT_FUNC(0, 8600, u16);
	for (u64 x = 0; x < WIDTH; x++)
		for (u64 y = 0; y < WIDTH; y++)
			data.SetHeightAt(x, y, dis(gen));

	Writer writer;
	BinaryStream stream;
	writer.Write(data, stream);
	// Dump the stream to a file
	std::ofstream file("uncompressed.dxob", std::ios::binary);
	file.write(reinterpret_cast<char*>(stream.data()), stream.size());
	file.close();
	settings.isGZipCompressed = true;
	data.SetFileSettings(settings);
	stream.clear();
	writer.Write(data, stream);
	file.open("compressed.dxob", std::ios::binary);
	file.write(reinterpret_cast<char*>(stream.data()), stream.size());
	file.close();



}

int main() {
	u64 failed = 0;
	if (!ReadAndWriteStandardBitSize())
		failed++;
	if (!ReadAndWriteSmallerStandardBitSize())
		failed++;
	if (!ReadAndWriteLargerStandardBitSize())
		failed++;
	if (!ReadAndWriteStandardBitSizeSigned())
		failed++;
	BenchmarkFindMaximumDelta();
	TestWriterAndConstructor();






	if (failed == 0)
		std::cout << "All tests passed" << std::endl;
	else
		std::cout << failed << " tests failed" << std::endl;
}
