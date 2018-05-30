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
        constexpr T* end() { return std::end(data); }
        constexpr const T* cend() const { return std::cend(data); }
        constexpr size_t size() const { return VSize; }
    };

    template<typename TCrc, size_t VSize, TCrc VPolynomial, TCrc VInitial, TCrc VFinalXor, bool VReflectInput, bool VReflectOutput, bool VDoCheck = false, TCrc VExpectedCheck = 0>
    class crc_base
    {
    public:
        static constexpr size_t table_size = 256;

        using crc_t = TCrc;
        //using table_t = std::array<crc_t, table_size>;    // C++17
        using table_t = array_wrapper<crc_t, table_size>;   // C++14

        static constexpr size_t size = VSize;
        static constexpr crc_t polynomial = VPolynomial;
        static constexpr crc_t initial = VInitial;
        static constexpr crc_t final_xor = VFinalXor;
        static constexpr bool reflect_input = VReflectInput;
        static constexpr bool reflect_output = VReflectOutput;
        static constexpr bool do_check = VDoCheck;
        static constexpr crc_t expected_check = VExpectedCheck;

    private:
        static constexpr crc_t msb_mask = static_cast<crc_t>(static_cast<crc_t>(1) << (size - 1));
        static constexpr crc_t full_mask = static_cast<crc_t>((static_cast<crc_t>(1) << size) - 1);

        static_assert(size > 0, "CRC size must be greater than 0");
        static_assert(msb_mask != 0, "The provided data type can't fit the CRC size requested");
        static_assert((polynomial & full_mask) == polynomial, "Polynomial is larger than the specified CRC size");
        static_assert((initial & full_mask) == initial, "Initial value is larger than the specified CRC size");
        static_assert((final_xor & full_mask) == final_xor, "Final xor value is larger than the specified CRC size");

        crc_t current;

        static constexpr uint8_t reflect_byte(uint8_t b, size_t len)
        {
            uint8_t result = 0;
            for(auto i = 0; i < len; ++i)
            {
                result <<= 1;
                result |= (b & 1);
                b >>= 1;
            }
            return result;
            
            /*b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
            b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
            b = (b & 0xAA) >> 1 | (b & 0x55) << 1;*/
            return b;
        }

        static constexpr crc_t reflect_crc(crc_t crc)
        {
            size_t remaining_size = size;
            crc_t reflected = 0;
            for(auto i = 0; i < size; i += 8)
            {
                size_t len = remaining_size < 8 ? remaining_size : 8;
                remaining_size -= len;
                auto byte = static_cast<uint8_t>(crc & 0xFF);
                auto reflected_byte = reflect_byte(byte, len);
                reflected <<= len;
                reflected |= reflected_byte;
                crc >>= len;
            }
            return reflected;
        }

        static constexpr crc_t corrected_initial = reflect_input ? reflect_crc(initial) : initial;

        static constexpr table_t calc_table()
        {
            table_t table{};
            table[0] = 0;
            crc_t crc = msb_mask;
            for (auto i = 1; i < table_size; i <<= 1)
            {
                if (crc & msb_mask)
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

            if (reflect_input)
            {
                table_t refl{};
                for (auto i = 0; i < table_size; ++i)
                {
                    auto refl_offset = reflect_byte(static_cast<uint8_t>(i), 8);
                    auto refl_entry = reflect_crc(table[refl_offset]);
                    refl[i] = refl_entry;
                }
                return refl;
            }

            return table;
        }

        template<typename TIt>
        static constexpr crc_t compute_checksum_core(crc_t crc, const TIt begin, const TIt end)
        {
            if (reflect_input)
            {
                for (auto it = begin; it != end; ++it)
                {
                    const auto data_byte = static_cast<uint8_t>(*it);
                    const auto offs = data_byte ^ (crc & 0xFF);
                    crc = ((crc >> 8) ^ table[offs]) & full_mask;
                }
            }
            else
            {
                constexpr auto shift = static_cast<int32_t>(size) - 8;
                if (shift < 0)
                {
                    for (auto it = begin; it != end; ++it)
                    {
                        const auto data_byte = static_cast<uint8_t>(*it);
                        const auto offs = data_byte ^ (crc << -shift);
                        crc = ((crc << 8) ^ table[offs]) & full_mask;
                    }
                }
                else
                {
                    for (auto it = begin; it != end; ++it)
                    {
                        const auto data_byte = static_cast<uint8_t>(*it);
                        const auto offs = data_byte ^ (crc >> shift);
                        crc = ((crc << 8) ^ table[offs]) & full_mask;
                    }
                }
            }
            return crc;
        }

        template<typename TIt>
        static constexpr crc_t compute_checksum_final(crc_t crc, const TIt begin, const TIt end)
        {
            constexpr auto do_reflect_output = reflect_input ^ reflect_output;
            if (do_reflect_output)
            {
                crc = reflect_crc(crc);
            }
            crc ^= final_xor;
            return crc;
        }

    public:
        static constexpr table_t table = calc_table();

        constexpr void set(crc_t new_val)
        {
            current = new_val;
        }

        constexpr void reset()
        {
            set(corrected_initial);
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
            current = compute_checksum_core(current, begin, end);
            auto result = compute_checksum_final(current, begin, end);
            return result;
        }

        template<typename TIt>
        static constexpr crc_t compute_checksum_static(const TIt begin, const TIt end)
        {
            auto intermediate = compute_checksum_core(corrected_initial, begin, end);
            auto result = compute_checksum_final(intermediate, begin, end);
            return result;
        }

        static constexpr array_wrapper<uint8_t, 9> check_input = { 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39 }; // 123456789 in ASCII
        static constexpr crc_t check = compute_checksum_static(check_input.cbegin(), check_input.cend());
        static_assert(!do_check || expected_check == check, "CRC check failed");
    };
}