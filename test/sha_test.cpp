#include <cleo/sha.hpp>
#include <gtest/gtest.h>

namespace cleo
{
namespace test
{

struct sha256hasher_test : testing::Test
{
    std::string hash(std::string s)
    {
        Sha256Hasher h;
        h.append(s.c_str(), s.size());
        h.finish();
        return to_string(h.hash());
    }
};

TEST_F(sha256hasher_test, hash)
{
    {
        Sha256Hasher h;
        h.finish();
        EXPECT_EQ("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855", to_string(h.hash()));
    }
    EXPECT_EQ("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855", hash(""));
    EXPECT_EQ("ca978112ca1bbdcafac231b39a23dc4da786eff8147c4e72b9807785afee48bb", hash("a"));
    EXPECT_EQ("fb8e20fc2e4c3f248c60c39bd652f3c1347298bb977b8b4d5903b85055620603", hash("ab"));
    EXPECT_EQ("ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad", hash("abc"));
    EXPECT_EQ("88d4266fd4e6338d13b845fcf289579d209c897823b9217da3e161936f031589", hash("abcd"));
    EXPECT_EQ("8215bf4bad3ee05a81e78d4628566ac654e24c1f448d67778e86086170b10ee0", hash("55 bytes jknbse5t8v230985nb2yo9ibn[64piwu0345vt9snge8vg"));
    EXPECT_EQ("d094e2427ff6af07c872c6f948a526a586a17c780286e581b1f36e07cb4f9ddd", hash("56 bytes jknbse5t8v230985nb2yo9ibn[64piwu0345vt9snge8vg9"));
    EXPECT_EQ("3e0bc541387bce676e3b256f11c663adf1be4af07a9e1507e4dec5e19ca0f4f6", hash("57 bytes jknbse5t8v230985nb2yo9ibn[64piwu0345vt9snge8vg9x"));
    EXPECT_EQ("24f527ea4bfd6be5b595191b9ab37bd0c4ac516d0cfa43b3c3a73b4fd13a601c", hash("63 bytes isdkfhbnvsiedwe45hb89n3475ghn9w2b35hnv298375hty2bn8345"));
    EXPECT_EQ("df90dae13f2fcf6aa9e3ba20d1f8291cd271a739f2575818b47713e6b1a8ca04", hash("64 bytes isdkfhbnvsiedwe45hb89n3475ghn9w2b35hnv298375hty2bn83452"));
    EXPECT_EQ("04255ea3a42964176238361334970fce004c020d66da50f4720ef06748b4bf2e", hash("65 bytes isdkfhbnvsiedwe45hb89n3475ghn9w2b35hnv298375hty2bn8345zz"));
    EXPECT_EQ("d75f917d14b478d8762e5eb9ddb728b4d21be22d7b5b4434e7a0fcdd20c14298", hash("119 bytes ksuiedjryhvignwsu59gbv283754y9ng8b273y5ntb827439n5tgvjedfhvowseighqw4395yu2q09843myn98q370vbnq83745tv98q3746t"));
    EXPECT_EQ("82f33df88e775c02addfdcc24aa91bc4732351d2c5b51970a1cbbbe21d35788c", hash("120 bytes ksuiedjryhvignwsu59gbv283754y9ng8b273y5ntb827439n5tgvjedfhvowseighqw4395yu2q09843myn98q370vbnq83745tv98q3746tx"));
    EXPECT_EQ("03118dc356bcf4292003a4f5d43c1cbfa068e8bde5f878565117ff43a337cd42", hash("121 bytes ksuiedjryhvignwsu59gbv283754y9ng8b273y5ntb827439n5tgvjedfhvowseighqw4395yu2q09843myn98q370vbnq83745tv98q3746tbb"));
}

}
}
