/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include "libp2p/multi/utils/base58_codec.hpp"
#include "common/buffer.hpp"
using libp2p::multi::Base58Codec;
using std::string_literals::operator""s;

/**
 * @given a byte array, e. g. a very big number or a string
 * @when encoding it into base58
 * @then a valid base58 string is the result
 */
TEST(Base58Codec, Encode) {
  ASSERT_EQ(Base58Codec::encode("hello world"), "StV1DL6CwTryKyV"s);
  ASSERT_EQ(Base58Codec::encode("\0\0hello world"s), "11StV1DL6CwTryKyV"s);
  ASSERT_EQ(Base58Codec::encode(""), ""s);
  ASSERT_EQ(Base58Codec::encode("aaaaaaaaaaaaaaaaaaaa"), "2Mgr6LxWNxjaW44RyRozxwbRxMrY"s);
}

TEST(Base58Codec, Decode) {
  kagome::common::Buffer res;
  ASSERT_NO_THROW({
                    res = Base58Codec::decode("StV1DL6CwTryKyV").value();
                  });
  ASSERT_EQ(std::string(res.begin(), res.end()), "hello world"s);

  ASSERT_NO_THROW({
                    res = Base58Codec::decode("11StV1DL6CwTryKyV").value();
                  });
  ASSERT_EQ(std::string(res.begin(), res.end()), "\0\0hello world"s);

  std::string enc = Base58Codec::encode("a");
  ASSERT_EQ(enc, "2g");
  kagome::common::Buffer res1;
  ASSERT_NO_THROW({
    res1 = Base58Codec::decode(enc).value();
  });
  // TODO(Harrm) decodes to '\0', because Base58Codec uses max size to decode
  ASSERT_NE(std::string(res1.begin(), res1.end()), "a"s);
  ASSERT_FALSE(Base58Codec::decode("DXstMaV43WpY,4ceREiiTv2UntmoiA9a"));

  ASSERT_EQ(Base58Codec::decode("").value(), kagome::common::Buffer{});
}

