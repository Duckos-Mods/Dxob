#pragma once
#include <concepts>
#include <memory>
#include "DxobTypes.hpp"
#include <cmath>
#include <span>
#include <algorithm>

// Doing (type << 1) >> 1 "jiggles" the last bit from the left off
// That doesnt work with signed types, so we need to do a different approach for them so CLIP_TOP_BIT is defined for signed types

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
            return u8(byteCount);

        }
        template<std::integral valueType>
        valueType constexpr CalculateAllButMaxBit()
        {
            valueType temp = 0;
			for (u8 i = 0; i < sizeof(valueType) * 8 - 1; i++)
				temp |= 1 << i;
			return temp;

        }
    }

    template<std::integral valueType = u16>
    class ByteArrayWrapper
    {
    public:
        ByteArrayWrapper(u64 Size = 4096, u8 bitsPerIndex = 8) {
            u8* temp = tcalloc<u8>(Intr::GetByteSize(Size, bitsPerIndex));
            if (temp == nullptr)
                DXOB_THROW(std::bad_alloc());
            this->m_data = temp;
            this->m_size = Size;
            this->bitsPerIndex = bitsPerIndex;
            this->totalBytesTaken = Intr::GetByteSize(Size, bitsPerIndex);
           // assert(bitsPerIndex % 8 != 0 && std::is_signed<valueType>::value); // Non standard bit sizes are not supported for signed types
            m_selectBitmask = calculateSelectBitmask();

        }
        ByteArrayWrapper(u8* data, u64 Size, u8 bitsPerIndex) {
			this->m_data = data;
			this->m_size = Size;
			this->bitsPerIndex = bitsPerIndex;
            assert(bitsPerIndex % 8 != 0 && std::is_signed<valueType>::value); // Non standard bit sizes are not supported for signed types
            m_selectBitmask =  calculateSelectBitmask();
		}
        ByteArrayWrapper(const ByteArrayWrapper& other)
        {
            u8* temp = tcalloc<u8>(Intr::GetByteSize(other.m_size, other.bitsPerIndex));
            if (temp == nullptr)
                DXOB_THROW(std::bad_alloc());
            this->m_data = temp;
			this->m_size = other.m_size;
			this->bitsPerIndex = other.bitsPerIndex;
			m_selectBitmask =  calculateSelectBitmask();
            assert(bitsPerIndex % 8 != 0 && std::is_signed<valueType>::value); // Non standard bit sizes are not supported for signed types
            memcpy(m_data, other.m_data, Intr::GetByteSize(other.m_size, other.bitsPerIndex));
		}
        ByteArrayWrapper(ByteArrayWrapper&& other)
        {
            this->m_data = other.m_data;
            this->m_size = other.Size;
            this->bitsPerIndex = other.bitsPerIndex;
            m_selectBitmask = other.m_selectBitmask;
            other.m_data = nullptr;
            other.m_size = 0;
            other.bitsPerIndex = 0;
            assert(bitsPerIndex % 8 != 0 && std::is_signed<valueType>::value); // Non standard bit sizes are not supported for signed types
            other.m_selectBitmask = 0;
        }
        ByteArrayWrapper(std::span<valueType> data, u8 bitsPerIndex = -1)
        {
			if (bitsPerIndex == -1)
                bitsPerIndex = CalculateMinBitsForValue(*std::max_element(data.begin(), data.end()));
            u8* temp = tcalloc<u8>(Intr::GetByteSize(data.size(), bitsPerIndex));
            if (temp == nullptr)
				DXOB_THROW(std::bad_alloc());
            this->m_data = temp;
            this->m_size = data.size();
            this->bitsPerIndex = bitsPerIndex;
            assert(bitsPerIndex % 8 != 0 && std::is_signed<valueType>); // Non standard bit sizes are not supported for signed types
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
        std::span<u8> GetData() {return std::span<u8>(m_data, m_size);}
		std::span<const u8> GetData() const {return std::span<const u8>(m_data, m_size);}
        u64 GetTotalBytesTaken() const {return totalBytesTaken;}
           
        u64 GetSize() const {return m_size;} // These get inlined cuz templates
        u8 GetBitsPerIndex() const {return bitsPerIndex;}
        valueType operator[](u64 index) { return NoGuardRead(index); }
        valueType NoGuardRead(u64 index, bool AvoidSlam = false)
        {
            if constexpr (std::is_signed<valueType>::value)
				return NoGuardReadSigned(index, AvoidSlam);
			else
                return NoGuardReadNonSigned(index, AvoidSlam);
        }
        void NoGuardWrite(u64 index, valueType val)
        {
            if constexpr (std::is_signed<valueType>::value)
                NoGuardWriteSigned(index, val);
            else
                NoGuardWriteNonSigned(index, val);
        }
   
        // Begin function
        valueType* begin() { return reinterpret_cast<valueType*>(m_data); }
        const valueType* begin() const { return reinterpret_cast<valueType*>(m_data); }
        // End function
        valueType* end() { return reinterpret_cast<valueType*>(m_data + Intr::GetByteSize(m_size, bitsPerIndex)); }
        const valueType* end() const { return reinterpret_cast<valueType*>(m_data + Intr::GetByteSize(m_size, bitsPerIndex)); }


        void SetUnderlyingData(u8* data, u64 Size, u8 bps)
        {
			free(m_data);
			this->m_data = data;
			this->m_size = Size;
			this->bitsPerIndex = bps;
            this->totalBytesTaken = Intr::GetByteSize(Size, bps);
			m_selectBitmask = calculateSelectBitmask();
		}
        template <std::integral T>
        void FromVector(std::span<T> bytes)
        {
            if (bytes.size() < m_size)
                DXOB_RUNTIME("Not enough values in the span");
            for (u64 i = 0; i < m_size; i++)
                NoGuardWrite(i, bytes[i]);
        }
private:
        constexpr valueType calculateSelectBitmask()
        {
            assert(bitsPerIndex != 0);
            valueType mask = 0;
			for (u8 i = 0; i < bitsPerIndex; i++)
				mask |= 1 << i;
			return mask;
		}
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
        valueType NoGuardReadNonSigned(u64 index, bool AvoidSlam = false, u64* rawWriteLoc = nullptr)
        {
            u64 startByte = u64(floor(index * bitsPerIndex / 8)); // Get the byte that the index starts in
            u64 startBit = (index * bitsPerIndex % 8);
            if (startBit == 8)
            {
                startBit = 0;
				startByte++;
            }
            u64 cpyCount = CalculateCpyCount(index);
            u64 value;
            u64 tempUnmasked = 0;
            memcpy(reinterpret_cast<void*>(&tempUnmasked), m_data + startByte, cpyCount);
            u64 maskClone = m_selectBitmask;
            maskClone <<= startBit;
            if (!AvoidSlam)
                value = (tempUnmasked & maskClone) >> startBit;
            else
                value = tempUnmasked;
            if (rawWriteLoc != nullptr)
                *rawWriteLoc = tempUnmasked;

            return valueType(value);
        }
        void NoGuardWriteNonSigned(u64 index, valueType val)
        {
            u64 ourVal = val;
            u64 startByte = u64(floor(index * bitsPerIndex / 8)); // Get the byte that the index starts in
            u64 startBit = (index * bitsPerIndex % 8);
            if (startBit == 8)
            {
                startBit = 0;
                startByte++;
            }
            u64 cpyCount = CalculateCpyCount(index);
            u64 orig = 0;
            NoGuardReadNonSigned(index, true, &orig); // Get the value at the index
            u64 maskClone = m_selectBitmask;
            u64 valClamped = ourVal & maskClone; // This should just take N bits from the value
            valClamped <<= startBit; // Shift the value to the start bit
            maskClone <<= startBit; // Shift the mask to the start bit
            u64 maskNeg = ~maskClone; // Get the inverse of the mask
            orig &= maskNeg; // Clear the bits that are going to be replaced
            orig |= valClamped; // Set the bits that are going to be replaced
            memcpy(m_data + startByte, &orig, cpyCount); // Write the value back to the data
        }
        
        void NoGuardWriteSigned(u64 index, valueType val)
        {
            valueType newVal;
            if (val < 0)
            {
                // Remove the sign bit
                newVal = CLIP_TOP_BIT(val);
                newVal |= 1 << (bitsPerIndex - 1);
            }
            else
            {
				newVal = val;
			}
            return NoGuardWriteNonSigned(index, newVal);
        }
        valueType NoGuardReadSigned(u64 index, bool AvoidSlam = false)
        {
            using USS = UnsignedSameSize<valueType>::type;
			valueType temp = NoGuardReadNonSigned(index, AvoidSlam);
            bool isSigned = temp & (1 << (bitsPerIndex - 1));
            if (isSigned)
            {
                // "Design" the value so its non signed
                // USS inverseTemp = ~temp; // Sets all on bits to 0 and all off bits to 1
                //u8 newSignBitLocation = bitsPerIndex - 1;
                USS newData = ~0; // Set all bits to 1
                // "smear" the sign bit over from the end to the real sign location to keep value
                newData &= temp;
                valueType returnData = 0;
                memcpy(&returnData, &newData, sizeof(valueType));
                return returnData;
            }
            else
				return valueType(temp);
		}


        
        u64 CalculateCpyCount(u64 index)
        {
            u64 startByte = u64(floor(index * bitsPerIndex / 8)); // Get the byte that the index starts in
            u64 startBit = index * bitsPerIndex % 8; // Get the bit that the index starts at
            u64 endByte = u64(floor((index + 1) * bitsPerIndex / 8)); // Get the byte that the index ends in
            u64 endBit = (index + 1) * bitsPerIndex % 8; // Get the bit that the index ends at
            u64 cpyCount = endByte - startByte;
            if (cpyCount < Intr::GetBytesForBitCount(bitsPerIndex))
                cpyCount = Intr::GetBytesForBitCount(bitsPerIndex);
            else if (endBit != 0)
                cpyCount++;
            else if (startBit == 8)
            {
                cpyCount++;
				startBit = 0;
            }
            return cpyCount;
        }
    private:
        u8* m_data;
        u64 m_size = 0;
        u64 m_selectBitmask = 0;
        u64 totalBytesTaken = 0;
        u8 bitsPerIndex = 0;

    };
}