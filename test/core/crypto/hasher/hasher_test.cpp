/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "common/blob.hpp"
#include "crypto/hasher/hasher_impl.hpp"

using kagome::common::Buffer;

/**
 * Hasher fixture
 */
class HasherFixture : public testing::Test {
 public:
  ~HasherFixture() override = default;

  HasherFixture() {
    hasher = std::make_shared<kagome::hash::HasherImpl>();
  }

  /**
   * useful function for convenience
   */
  template <int size>
  static Buffer blob2buffer(const kagome::common::Blob<size> &blob) noexcept {
    Buffer out;
    out.put({blob.data(), static_cast<long>(blob.size())});
    return out;
  }

  static Buffer string2buffer(const std::string_view &view) noexcept {
    Buffer out;
    out.put(view);
    return out;
  }

 protected:
  std::shared_ptr<kagome::hash::Hasher> hasher;
};

/**
 * @given empty source value
 * @when Hasher::hashTwox_128 method is applied
 * @then expected result obtained
 */
TEST_F(HasherFixture, twox_128_empty) {
  auto hash = hasher->twox_128({});
  std::vector<uint8_t> match = {153, 233, 216, 81, 55, 219, 70,  239,
                                75,  190, 163, 54, 19, 186, 175, 213};

  ASSERT_EQ(blob2buffer<16>(hash).toVector(), match);
}

/**
 * @given some common source value
 * @when Hasher::hashTwox_128 method is applied
 * @then expected result obtained
 */
TEST_F(HasherFixture, twox_128_some) {
  auto hash = hasher->twox_128({0x41, 0x42, 0x43, 0x44, 0x45, 0x46});
  std::vector<uint8_t> match = {184, 65, 176, 250, 243, 129, 181, 3,
                                77,  82, 63,  150, 129, 221, 191, 251};
  ASSERT_EQ(blob2buffer<16>(hash).toVector(), match);
}

/**
 * @given empty source value
 * @when Hasher::hashTwox_256 method is applied
 * @then expected result obtained
 */
TEST_F(HasherFixture, twox_256_empty) {
  auto hash = hasher->twox_256({});
  std::vector<uint8_t> match = {153, 233, 216, 81,  55,  219, 70,  239,
                                75,  190, 163, 54,  19,  186, 175, 213,
                                111, 150, 60,  100, 177, 243, 104, 90,
                                78,  180, 171, 214, 127, 246, 32,  58};
  ASSERT_EQ(blob2buffer<32>(hash).toVector(), match);
}

/**
 * @given some common source value
 * @when Hasher::hashTwox_256 method is applied
 * @then expected result obtained
 */
TEST_F(HasherFixture, twox_256_some) {
  // some value
  auto hash = hasher->twox_256({0x41, 0x42, 0x43, 0x44, 0x45, 0x46});
  std::vector<uint8_t> match = {184, 65,  176, 250, 243, 129, 181, 3,
                                77,  82,  63,  150, 129, 221, 191, 251,
                                33,  226, 149, 136, 6,   232, 81,  118,
                                200, 28,  69,  219, 120, 179, 208, 237};
  ASSERT_EQ(blob2buffer<32>(hash).toVector(), match);
}

/**
 * @given empty source value
 * @when Hasher::hashSha2_256 method is applied
 * @then expected result obtained
 */
TEST_F(HasherFixture, sha2_256_empty) {
  auto hash = hasher->sha2_256({});
  std::string_view match =
      "E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855";
  ASSERT_EQ(hasher->sha2_256({}).toHex(), match);
}

/**
 * @given simple source value
 * @when Hasher::hashSha2_256 method is applied
 * @then expected result obtained
 */
TEST_F(HasherFixture, sha2_256_simple) {
  // simple value
  auto hash = hasher->sha2_256(string2buffer("abc"));
  std::string_view match =
      "BA7816BF8F01CFEA414140DE5DAE2223B00361A396177A9CB410FF61F20015AD";
  ASSERT_EQ(hash.toHex(), match);
}

/**
 * @given some common source value
 * @when Hasher::hashSha2_256 method is applied
 * @then expected result obtained
 */
TEST_F(HasherFixture, sha2_256_common) {
  auto hash = hasher->sha2_256(string2buffer(
      "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"));
  std::string_view match =
      "248D6A61D20638B8E5C026930C3E6039A33CE45964FF2167F6ECEDD419DB06C1";
  ASSERT_EQ(hash.toHex(), match);
}

/**
 * @given some common source value
 * @when Hasher::hashBlake2_256 method is applied
 * @then expected result obtained
 */
TEST_F(HasherFixture, blake2_256) {
  Buffer buffer = {0x69, 0x20, 0x61, 0x6d, 0x20, 0x64, 0x61, 0x74, 0x61};
  std::vector<uint8_t> match = {0xba, 0x67, 0x33, 0x6e, 0xfd, 0x6a, 0x3d, 0xf3,
                                0xa7, 0x0e, 0xeb, 0x75, 0x78, 0x60, 0x76, 0x30,
                                0x36, 0x78, 0x5c, 0x18, 0x2f, 0xf4, 0xcf, 0x58,
                                0x75, 0x41, 0xa0, 0x06, 0x8d, 0x09, 0xf5, 0xb2};

  auto hash = hasher->blake2_256(buffer);
  ASSERT_EQ(blob2buffer<32>(hash).toVector(), match);
}
