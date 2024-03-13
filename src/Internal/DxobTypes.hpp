#pragma once
#include <concepts>
#include <vector>
#include "Defines.hpp"
#include <iostream>
#include <span>
namespace Dxob
{
    using u8 = unsigned char;
    using u16 = unsigned short;
    using u32 = unsigned int;
    using u64 = unsigned long long;

    using i8 = signed char;
    using i16 = signed short;
    using i32 = signed int;
    using i64 = signed long long;
    
    template<typename T>
    concept DataStore = requires(T t)
    {
        { t.size() } -> std::same_as<u64>;
        { t.operator[](u64{}) };
        { t.push_back({}) } -> std::same_as<void>;
    };
    #ifndef DXOB_DATASTORE
    #define DXOB_DATASTORE std::vector
    #endif

    static_assert(DataStore<DXOB_DATASTORE<u8>>, "DXOB_DATASTORE must satisfy the DataStore concept");

    class BinaryStream : public std::vector<u8> {
    private:
        u64 m_offset = 0;
    public:
        BinaryStream(u64 buffInitSize = 1024) : std::vector<u8>() { this->reserve(buffInitSize); }
        BinaryStream(const u8* data, u64 size) : std::vector<u8>(data, data + size) {}

        template<typename T>
        BinaryStream& write(const T* val, u64 size = -1)
        {
            if (size == -1) [[likely]]
				size = sizeof(T);
            const u8* data = reinterpret_cast<const u8*>(val);
			this->insert(this->end(), data, data + size);
			return *this;
		}

        BinaryStream& write(const u8* data, u64 size)
        {
            this->insert(this->end(), data, data + size);
            return *this;
        }
        BinaryStream& write(const u8* data, u64 size, u64 offset)
        {
			if (offset + size > this->size())
				this->resize(offset + size);
			for (u64 i = 0; i < size; i++)
				this->at(offset + i) = data[i];
            return *this;
        }
        BinaryStream& write(std::span<u8> data)
        {
			this->insert(this->end(), data.begin(), data.end());
            return *this;
        }
        template<std::integral T>
        BinaryStream& write(std::span<T> data)
        {
            u64 byteSize = data.size() * sizeof(T);
            this->reserve(this->size() + byteSize);
            this->insert(this->end(), reinterpret_cast<u8*>(data.data()), reinterpret_cast<u8*>(data.data()) + byteSize);
            return *this;
        }

        u8& operator[](u64 index)
        {
			return this->at(index);
		}

        BinaryStream& operator<< (u8 val)
        {
            this->push_back(val);
            return *this;
        }

        BinaryStream& operator<< (std::span<u8> val)
        {
            this->insert(this->end(), val.begin(), val.end());
			return *this;
        }

        template<typename T>
        BinaryStream& read(T* location, u64 size = -1)
        {
            if (size == -1) [[likely]]
                size = sizeof(T);
            u8* data = reinterpret_cast<u8*>(location);
            for (u64 i = 0; i < size; i++)
                data[i] = this->at(m_offset + i);
            m_offset += size;
            return *this;
        }

        BinaryStream& read(u8* location, u64 size)
        {
			for (u64 i = 0; i < size; i++)
				location[i] = this->at(m_offset + i);
			m_offset += size;
		    return *this;
        }

        void peakn(u8* location, u64 size)
        {
            for (u64 i = 0; i < size; i++)
                location[i] = this->at(m_offset + i);
        }

        template<typename T>
        BinaryStream& peakn(T* location, u64 size = -1)
        {
            if (size == -1) [[likely]]
				size = sizeof(T);
            u8* data = reinterpret_cast<u8*>(location);
            for (u64 i = 0; i < size; i++)
				data[i] = this->at(m_offset + i);
			return *this;
		}

        u64 tellg() const { return m_offset; }
        void seekg(u64 offset) { m_offset = offset; }



    };
    #include <type_traits>

    template <typename T>
    struct UnsignedSameSize;

    template <>
    struct UnsignedSameSize<int8_t> {
        using type = uint8_t;
    };

    template <>
    struct UnsignedSameSize<int16_t> {
        using type = uint16_t;
    };

    template <>
    struct UnsignedSameSize<int32_t> {
        using type = uint32_t;
    };

    template <>
    struct UnsignedSameSize<int64_t> {
        using type = uint64_t;
    };


}