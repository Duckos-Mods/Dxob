#pragma once
#include <concepts>
#include <memory>
#include "DxobTypes.hpp"
#include <cmath>
#include <span>
#include <algorithm>

namespace Dxob
{
    namespace Intr
    {
        u64 constexpr GetByteSize(u64 size, u8 bitsPerIndex)
        {
            u64 rawSize = size * bitsPerIndex;
            u64 byteSize = rawSize / 8;
            while (byteSize % 8 != 0)
                byteSize++;
            return byteSize;
        }

        u8 constexpr GetBytesForBitCount(u64 bitCount)
        {
            u64 remainder = bitCount % 8;
            if (remainder != 0)
            {
                while (remainder != 0)
                {
                    bitCount++;
                    remainder = bitCount % 8;
                }
            }
            u64 byteCount = bitCount / 8;
            return byteCount;

        }
    }

    template<std::integral valueType = u16>
    class ByteArrayWrapper
    {
    public:
        ByteArrayWrapper(u64 Size = 4096, u8 bitsPerIndex = 8) {
            void* temp = calloc(sizeof(u8), Intr::GetByteSize(Size, bitsPerIndex));
            if (temp == nullptr)
                throw std::bad_alloc();
            this->m_data = reinterpret_cast<u8*>(temp);
            this->Size = Size;
            this->bitsPerIndex = bitsPerIndex;
            m_selectBitmask =  calculateSelectBitmask();
        }
        ByteArrayWrapper(u8* data, u64 Size, u8 bitsPerIndex) {
			this->m_data = data;
			this->Size = Size;
			this->bitsPerIndex = bitsPerIndex;
			m_selectBitmask =  calculateSelectBitmask();
		}
        ByteArrayWrapper(const ByteArrayWrapper& other)
        {
            void* temp = malloc(Intr::GetByteSize(Size, bitsPerIndex));
            if (temp == nullptr)
                throw std::bad_alloc();
            this->m_data = reinterpret_cast<u8*>(temp);
			this->Size = other.Size;
			this->bitsPerIndex = other.bitsPerIndex;
			m_selectBitmask =  calculateSelectBitmask();
			memcpy(m_data, other.m_data, Intr::GetByteSize(other.Size, other.bitsPerIndex));
		}
        ByteArrayWrapper(ByteArrayWrapper&& other)
        {
            this->m_data = other.m_data;
            this->Size = other.Size;
            this->bitsPerIndex = other.bitsPerIndex;
            m_selectBitmask = other.m_selectBitmask;
            other.m_data = nullptr;
            other.Size = 0;
            other.bitsPerIndex = 0;
            other.m_selectBitmask = 0;
        }
        ByteArrayWrapper(std::span<valueType> data, u8 bitsPerIndex = -1)
        {
			if (bitsPerIndex == -1)
                bitsPerIndex = CalculateMinBitsForValue(*std::max_element(data.begin(), data.end()));
            void* temp = malloc(Intr::GetByteSize(data.size(), bitsPerIndex));
            if (temp == nullptr)
				throw std::bad_alloc();
            this->m_data = reinterpret_cast<u8*>(temp);
            this->Size = data.size();
            this->bitsPerIndex = bitsPerIndex;
            m_selectBitmask = calculateSelectBitmask();
            for (u64 i = 0; i < data.size(); i++)
				NoGuardWrite(i, data[i]);
        }
        ByteArrayWrapper& operator=(const ByteArrayWrapper& other)
        {
            if (this != &other)
            {
				delete[] m_data;
				this->m_data = new u8[Intr::GetByteSize(other.Size, other.bitsPerIndex)];
				this->Size = other.Size;
				this->bitsPerIndex = other.bitsPerIndex;
				m_selectBitmask =  calculateSelectBitmask();
				memcpy(m_data, other.m_data, Intr::GetByteSize(other.Size, other.bitsPerIndex));
			}
			return *this;
		}
        ByteArrayWrapper& operator=(ByteArrayWrapper&& other)
        {
            if (this != &other)
            {
				delete[] m_data;
				this->m_data = other.m_data;
				this->Size = other.Size;
				this->bitsPerIndex = other.bitsPerIndex;
				m_selectBitmask = other.m_selectBitmask;
				other.m_data = nullptr;
				other.Size = 0;
				other.bitsPerIndex = 0;
				other.m_selectBitmask = 0;
			}
			return *this;
		}
        ~ByteArrayWrapper()
        {
            free(m_data);
		}

        u8* GetDataRaw() {return m_data;}
        const u8* GetDataRaw() const {return m_data;}
        u64 GetSize() const {return Size;} // These get inlined cuz templates
        u8 GetBitsPerIndex() const {return bitsPerIndex;}
        valueType operator[](u64 index) { return NoGuardRead(index); }
        valueType NoGuardRead(u64 index, bool AvoidSlam = false)
        {
            u64 startByte = floor(index * bitsPerIndex / 8); // Get the byte that the index starts in
            u64 startBit = index * bitsPerIndex % 8; // Get the bit that the index starts at

            u64 endByte = floor((index + 1) * bitsPerIndex / 8); // Get the byte that the index ends in
            u64 endBit = (index + 1) * bitsPerIndex % 8; // Get the bit that the index ends at

            valueType value;

            valueType tempUnmasked = 0;
            u64 cpyCount = endByte - startByte;
            if (cpyCount < Intr::GetBytesForBitCount(bitsPerIndex))
                cpyCount = Intr::GetBytesForBitCount(bitsPerIndex);
            else if (endBit != 0)
                cpyCount++;
            memcpy(reinterpret_cast<void*>(&tempUnmasked), m_data + startByte, cpyCount);
            valueType maskClone = m_selectBitmask;
            maskClone <<= startBit;
            if (!AvoidSlam)
                value = (tempUnmasked & maskClone) >> startBit;
            else
                value = tempUnmasked;
            return value;
        }
        void NoGuardWrite(u64 index, valueType val)
        {
            u64 startByte = floor(index * bitsPerIndex / 8); // Get the byte that the index starts in
			u64 startBit = index * bitsPerIndex % 8; // Get the bit that the index starts at
            u64 endByte = floor((index + 1) * bitsPerIndex / 8); // Get the byte that the index ends in
            u64 endBit = (index + 1) * bitsPerIndex % 8; // Get the bit that the index ends at
            u64 cpyCount = endByte - startByte;
            if (cpyCount < Intr::GetBytesForBitCount(bitsPerIndex))
                cpyCount = Intr::GetBytesForBitCount(bitsPerIndex);
            else if (endBit != 0)
				cpyCount++;
            valueType orig = NoGuardRead(index, true); // Get the value at the index
            valueType maskClone = m_selectBitmask;
            valueType valClamped = val & maskClone; // This should just take N bits from the value
            valClamped <<= startBit; // Shift the value to the start bit
            maskClone <<= startBit; // Shift the mask to the start bit
            valueType maskNeg = ~maskClone; // Get the inverse of the mask
            orig &= maskNeg; // Clear the bits that are going to be replaced
            orig |= valClamped; // Set the bits that are going to be replaced
            memcpy(m_data + startByte, &orig, cpyCount); // Write the value back to the data
        }
   
        // Begin function
        valueType* begin() { return reinterpret_cast<valueType*>(m_data); }
        const valueType* begin() const { return reinterpret_cast<valueType*>(m_data); }
        // End function
        valueType* end() { return reinterpret_cast<valueType*>(m_data + Intr::GetByteSize(Size, bitsPerIndex)); }
        const valueType* end() const { return reinterpret_cast<valueType*>(m_data + Intr::GetByteSize(Size, bitsPerIndex)); }



 private:
        constexpr valueType calculateSelectBitmask()
        {
            assert(bitsPerIndex != 0);
            valueType mask = 0;
			for (u8 i = 0; i < bitsPerIndex; i++)
				mask |= 1 << i;
			return mask;
		}
    private:
        u8 CalculateMinBitsForValue(valueType value)
        {
			u8 bits = 0;
            while (value > 0)
            {
				value >>= 1;
				bits++;
			}
			return bits;
		}
    private:
        u8* m_data;
        u64 Size = 0;
        valueType m_selectBitmask = 0;
        u8 bitsPerIndex = 0;

    };
}