#include <array>
#include <cstdint>
#include <string>

namespace cleo
{

using Sha256Hash = std::array<std::uint8_t, 32>;

std::string to_string(const Sha256Hash& h);

class Sha256Hasher
{
public:
    Sha256Hasher() = default;
    void append(const char *bytes, std::size_t size);
    void append(char b);
    void finish();
    Sha256Hash hash() const;
private:
    std::array<std::uint8_t, 64> buffer{};
    std::uint8_t buffer_size{0};
    std::uint64_t data_size{0};
    std::array<std::uint32_t, 8> state{0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};

    void consume_buffer();
};

}
