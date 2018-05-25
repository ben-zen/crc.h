# crc.h
Single-header modern C++ library for computing CRC checksums

## Motivation

I recently started a new project, which required both CRC16 and CRC32. This resulted in two implementations, and was rather ugly. So I decided to practice some modern C++ and `constexpr`, and write a library which supports multiple configurations. 

## Usage

The main class is:
```cpp
// Defined in crc.h
crc::crc_base<data_t, length, polynomial, initial_value, final_xor_value, reflect_input, reflect_output>
```

By providing templated arguments, you can construct any CRC implementation. For example:

```cpp
using crc8_standard = crc::crc_base<uint8_t, 8, 0x07, 0x0, 0x0, false, false>;
using crc16_ccitt = crc::crc_base<uint16_t, 16, 0x1021, 0x0, 0x0, false, false>;
using crc32_standard = crc::crc_base<uint32_t, 32, 0x4C11DB7, 0xFFFFFFFF, 0xFFFFFFFF, true, true>;
```

The usage is then quite simple:

```cpp
crc16_ccitt crc;
const std::array<uint8_t, 5> data = { 1, 2, 3, 4, 5 };
uint16_t result1 = crc.compute_checksum(data.begin(), data.end()); // 0x8208 
uint16_t result2 = crc.compute_checksum(data, 0, 5); // Offset/length also supported
```

The library uses a 256-length lookup table to compute 8 bits of the checksum at the same time for performance reasons. As a result, length of only multiples of 8 are supported.