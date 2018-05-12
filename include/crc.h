#pragma once

#include <iterator>

namespace crc
{
    template<typename T, size_t VSize>
    class array_wrapper
    {
    public:
        T data[VSize];
        constexpr T& operator[] (const size_t index) { return data[index]; }
        constexpr const T& operator[] (const size_t index) const { return data[index]; }
        constexpr T* begin() { return std::begin(data); }
        constexpr const T* cbegin() const { return std::cbegin(data); }
        constexpr const T* cend() const { return std::cend(data); }
        constexpr size_t size() const { return VSize; }
    };

    template<typename TCrc, size_t VSize, TCrc VPolynomial, TCrc VInitial, TCrc VFinalXor>
    class crc_base
    {
        static_assert(VSize <= sizeof(TCrc) * 8, "CRC size can't be greater than the size of the data type");
        static_assert(VSize > 0, "CRC size must be greater than 0");
        static_assert(VSize % 8 == 0, "CRC size must be a multiple of 8");

    public:
        static constexpr size_t table_size = 256;
        using crc_t = TCrc;
        //using table_t = std::array<crc_t, table_size>;    // C++17
        using table_t = array_wrapper<crc_t, table_size>;   // C++14

    private:
        crc_t current;

        static constexpr table_t calc_table()
        {
            table_t table{};
            table[0] = 0;
            constexpr crc_t sb_mask = 1 << (size - 1);
            constexpr crc_t full_mask = size < sizeof(crc_t) * 8 ? (static_cast<crc_t>(1) << size) - 1 : ~static_cast<crc_t>(0);
            crc_t crc = sb_mask;
            for (auto i = 1; i < table_size; i <<= 1)
            {
                if (crc & sb_mask)
                {
                    crc <<= 1;
                    crc ^= polynomial;
                }
                else
                {
                    crc <<= 1;
                }
                crc &= full_mask;
                for (auto j = 0; j < i; ++j)
                {
                    table[i + j] = crc ^ table[j];
                }
            }
            return table;
        }

    public:
        static constexpr size_t size = VSize;
        static constexpr crc_t polynomial = VPolynomial;
        static constexpr crc_t initial = VInitial;
        static constexpr crc_t final_xor = VFinalXor;
        static constexpr table_t table = calc_table();

    public:
        constexpr void set(crc_t new_val)
        {
            current = new_val;
        }

        constexpr void reset()
        {
            set(initial);
        }

        constexpr crc_base() : current()
        {
            reset();
        }

        template<typename TContainer>
        crc_t compute_checksum(const TContainer container)
        {
            const auto begin = std::cbegin(container);
            const auto end = std::cend(container);
            return compute_checksum(begin, end);
        }

        template<typename TContainer>
        crc_t compute_checksum(const TContainer container, const size_t offset)
        {
            const auto begin = std::cbegin(container) + offset;
            const auto end = std::cend(container);
            return compute_checksum(begin, end);
        }

        template<typename TContainer>
        crc_t compute_checksum(const TContainer container, const size_t offset, const size_t length)
        {
            const auto begin = std::cbegin(container) + offset;
            const auto end = begin + length;
            return compute_checksum(begin, end);
        }

        template<typename TIt>
        crc_t compute_checksum(const TIt begin, const TIt end)
        {
            constexpr auto shift = size - 8;
            for (auto it = begin; it != end; ++it)
            {
                const auto& data = *it;
                //assert(data < table_size);
                const auto data_byte = static_cast<uint8_t>(data);
                const auto offs = data_byte ^ (current >> shift);
                current = (current << 8) ^ table[offs];
            }
            //current ^= final_xor;
            auto result = current;
            result ^= final_xor;
            return result;
        }
    };
}