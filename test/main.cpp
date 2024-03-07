#include <iostream>
#include "Dxob.hpp"
#include <random>
#include <vector>
#include <span>
#include <functional>
#include <string>
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
	RANDOM_INT_FUNC(0, 511, u16);
	ByteArrayWrapper<u16> data(4096, 9);
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

int main() {
	u64 failed = 0;
	if (!ReadAndWriteStandardBitSize())
		failed++;
	if (!ReadAndWriteSmallerStandardBitSize())
		failed++;
	if (!ReadAndWriteLargerStandardBitSize())
		failed++;
	MULTI_TEST(ReadAndWriteStandardBitSize, 10000);
	MULTI_TEST(ReadAndWriteSmallerStandardBitSize, 10000);
	MULTI_TEST(ReadAndWriteLargerStandardBitSize, 10000);
	std::cout << "Failed: " << failed << std::endl;
}
