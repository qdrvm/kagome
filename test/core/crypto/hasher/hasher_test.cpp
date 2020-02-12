/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "common/blob.hpp"
#include "crypto/hasher/hasher_impl.hpp"
#include "testutil/literals.hpp"

using kagome::common::Buffer;

/**
 * Hasher fixture
 */
class HasherFixture : public testing::Test {
 public:
  ~HasherFixture() override = default;

  HasherFixture() {
    hasher = std::make_shared<kagome::crypto::HasherImpl>();
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
  std::shared_ptr<kagome::crypto::Hasher> hasher;
};

/**
 * @given pre-known source value
 * @when Hasher::twox_64 method is applied
 * @then expected result obtained
 */
TEST_F(HasherFixture, twox_64) {
  auto hash = hasher->twox_64(Buffer().put("foo"));

  // match is output obtained from substrate
  kagome::common::Blob<8> match;
  match[0] = '?';
  match[1] = '\xba';
  match[2] = '\xc4';
  match[3] = 'Y';
  match[4] = '\xa8';
  match[5] = '\0';
  match[6] = '\xbf';
  match[7] = '3';

  ASSERT_EQ(hash, match);
}

/**
 * @given some common source value
 * @when Hasher::twox_128 method is applied
 * @then expected result obtained
 */
TEST_F(HasherFixture, twox_128) {
  auto hash = hasher->twox_128(Buffer{"414243444546"_unhex});
  std::vector<uint8_t> match = {
      184, 65, 176, 250, 243, 129, 181, 3, 77, 82, 63, 150, 129, 221, 191, 251};
  ASSERT_EQ(blob2buffer<16>(hash).toVector(), match);
}

/**
 * @given some common source value
 * @when Hasher::twox_256 method is applied
 * @then expected result obtained
 */
TEST_F(HasherFixture, twox_256) {
  // some value
  auto v = Buffer{0x41, 0x42, 0x43, 0x44, 0x45, 0x46};
  auto hash = hasher->twox_256(v);
  std::vector<uint8_t> match = {184, 65,  176, 250, 243, 129, 181, 3,
                                77,  82,  63,  150, 129, 221, 191, 251,
                                33,  226, 149, 136, 6,   232, 81,  118,
                                200, 28,  69,  219, 120, 179, 208, 237};
  ASSERT_EQ(blob2buffer<32>(hash).toVector(), match);
}

/**
 * @given some common source value
 * @when Hasher::sha2_256 method is applied
 * @then expected result obtained
 */
TEST_F(HasherFixture, sha2_256) {
  auto hash = hasher->sha2_256(string2buffer(
      "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"));
  std::string_view match =
      "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1";
  ASSERT_EQ(hash.toHex(), match);
}

/**
 * @given some common source value
 * @when Hasher::blake2_256 method is applied
 * @then expected result obtained
 */
TEST_F(HasherFixture, blake2_256) {
  Buffer buffer{"6920616d2064617461"_unhex};
  std::vector<uint8_t> match =
      "ba67336efd6a3df3a70eeb757860763036785c182ff4cf587541a0068d09f5b2"_unhex;

  auto hash = hasher->blake2b_256(buffer);
  ASSERT_EQ(blob2buffer<32>(hash).toVector(), match);
}
