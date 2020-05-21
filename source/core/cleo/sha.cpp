#include "sha.hpp"
#include <algorithm>

namespace cleo
{

namespace
{

constexpr std::array<std::uint32_t, 64> k = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

std::uint32_t ror(std::uint32_t x, std::int8_t n)
{
    return (x >> n) | (x << (32 - n));
}

}

std::string to_string(const Sha256Hash& h)
{
    std::string s(h.size() * 2, '\0');
    auto hexdigit = "0123456789abcdef";
    for (std::size_t i = 0; i != h.size(); ++i)
    {
        auto b = h[i];
        s[i * 2] = hexdigit[b >> 4];
        s[i * 2 + 1] = hexdigit[b & 0xf];
    }
    return s;
}

void Sha256Hasher::append(char b)
{
    assert(buffer_size < 64);
    buffer[buffer_size++] = b;
    ++data_size;
    if (buffer_size == 64)
        consume_buffer();
}

void Sha256Hasher::append(const char *bytes, std::size_t size)
{
    for (; size; --size)
        append(*bytes++);
}

void Sha256Hasher::finish()
{
    assert(buffer_size < 64);
    buffer[buffer_size++] = 0x80;
    std::uint8_t SIZE_WO_LEN = 56;
    if (buffer_size > SIZE_WO_LEN)
    {
        std::fill(buffer.begin() + buffer_size, buffer.end(), 0);
        consume_buffer();
    }
    std::fill(buffer.begin() + buffer_size, buffer.begin() + SIZE_WO_LEN, 0);
    std::uint64_t bit_size = data_size * 8;
    for (std::uint8_t i = 0; i != 8; ++i)
        buffer[56 + i] = bit_size >> (56 - i * 8);
    consume_buffer();
}

Sha256Hash Sha256Hasher::hash() const
{
    Sha256Hash h;
    for (std::size_t i = 0; i != 32; ++i)
        h[i] = state[i >> 2] >> ((~i & 3) << 3);
    return h;
}

void Sha256Hasher::consume_buffer()
{
    buffer_size = 0;

    std::array<std::uint32_t, 64> w;

    for (std::uint8_t i = 0; i != 16; ++i)
        w[i] = (buffer[(i << 2)] << 24) | (buffer[(i << 2) + 1] << 16) | (buffer[(i << 2) + 2] << 8) | buffer[(i << 2) + 3];

    for (std::uint8_t i = 16; i != 64; ++i)
    {
        auto s0 = ror(w[i - 15], 7) ^ ror(w[i - 15], 18) ^ (w[i - 15] >> 3);
        auto s1 = ror(w[i - 2], 17) ^ ror(w[i - 2], 19) ^ (w[i - 2] >> 10);
        w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }

    auto cs = state;
    for (std::uint8_t i = 0; i != 64; ++i)
    {
        auto S0 = ror(cs[0], 2) ^ ror(cs[0], 13) ^ ror(cs[0], 22);
        auto S1 = ror(cs[4], 6) ^ ror(cs[4], 11) ^ ror(cs[4], 25);
        auto ch = (cs[4] & cs[5]) ^ (~cs[4] & cs[6]);
        auto maj = (cs[0] & cs[1]) ^ (cs[0] & cs[2]) ^ (cs[1] & cs[2]);
        auto temp1 = cs[7] + S1 + ch + k[i] + w[i];
        auto temp2 = S0 + maj;

        cs[7] = cs[6]; cs[6] = cs[5]; cs[5] = cs[4];
        cs[4] = cs[3] + temp1;
        cs[3] = cs[2]; cs[2] = cs[1]; cs[1] = cs[0];
        cs[0] = temp1 + temp2;
    }

    for (std::uint8_t i = 0; i != 8; ++i)
        state[i] += cs[i];
}

}
