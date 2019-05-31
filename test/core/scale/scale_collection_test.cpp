/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "scale/scale.hpp"
#include "testutil/outcome.hpp"

using kagome::scale::BigInteger;
using kagome::scale::ByteArray;
using kagome::scale::encode;
using kagome::scale::ScaleDecoderStream;
using kagome::scale::ScaleEncoderStream;

using kagome::scale::decode;
using kagome::scale::encode;

/**
 * @given collection of 80 items of type uint8_t
 * @when encodeCollection is applied
 * @then expected result is obtained: header is 2 byte, items are 1 byte each
 */
TEST(Scale, encodeCollectionOf80) {
  // 80 items of value 1
  ByteArray collection(80, 1);
  auto match = ByteArray{65, 1};  // header
  match.insert(match.end(), collection.begin(), collection.end());
  ScaleEncoderStream s;
  ASSERT_NO_THROW((s << collection));
  auto &&out = s.data();
  ASSERT_EQ(out.size(), 82);
  ASSERT_EQ(out, match);
}

/**
 * @given collection of items of type uint16_t
 * @when encodeCollection is applied
 * @then expected result is obtained
 */
TEST(Scale, encodeCollectionUint16) {
  std::vector<uint16_t> collection = {1, 2, 3, 4};
  ScaleEncoderStream s;
  ASSERT_NO_THROW((s << collection));
  auto &&out = s.data();
  // clang-format off
  ASSERT_EQ(out,
          (ByteArray{
              16,  // header
            1, 0,  // first item
            2, 0,  // second item
            3, 0,  // third item
            4, 0  // fourth item
              }));
  // clang-format on
}

/**
 * @given collection of items of type uint32_t
 * @when encodeCollection is applied
 * @then expected result is obtained
 */
TEST(Scale, encodeCollectionUint32) {
  std::vector<uint32_t> collection = {50462976, 117835012, 185207048,
                                      252579084};
  ScaleEncoderStream s;
  ASSERT_NO_THROW((s << collection));
  auto &&out = s.data();
  // clang-format off
  ASSERT_EQ(out,
            (ByteArray{
                    16,                // header
                    0, 1, 2, 3,        // first item
                    4, 5, 6, 7,        // second item
                    8, 9, 0xA, 0xB,    // third item
                    0xC, 0xD, 0xE, 0xF // fourth item
            }));
  // clang-format on
}

/**
 * @given collection of items of type uint64_t
 * @when encodeCollection is applied
 * @then expected result is obtained
 */
TEST(Scale, encodeCollectionUint64) {
  std::vector<uint64_t> collection = {506097522914230528ull,
                                      1084818905618843912ull};
  ScaleEncoderStream s;
  ASSERT_NO_THROW((s << collection));
  auto &&out = s.data();
  // clang-format off
  ASSERT_EQ(out,
            (ByteArray{
                    8,                // header
                    0, 1, 2, 3,        // first item
                    4, 5, 6, 7,        // second item
                    8, 9, 0xA, 0xB,    // third item
                    0xC, 0xD, 0xE, 0xF // fourth item
            }));
  // clang-format on
}

/**
 * @given collection of items of type uint16_t containing 2^14 items
 * where collection[i]  == i % 256
 * @when encodeCollection is applied
 * @then obtain byte array of length 32772 bytes
 * where each second byte == 0 and collection[(i-4)/2] == (i/2) % 256
 */
TEST(Scale, encodeLongCollectionUint16) {
  std::vector<uint16_t> collection;
  auto length = 16384;
  collection.reserve(length);
  for (auto i = 0; i < length; ++i) {
    collection.push_back(i % 256);
  }

  ScaleEncoderStream s;
  ASSERT_NO_THROW((s << collection));
  auto &&out = s.data();
  ASSERT_EQ(out.size(), (length * 2 + 4));

  // header takes 4 byte,
  // first 4 bytes represent le-encoded value 2^16 + 2
  // which is compact-encoded value 2^14 = 16384
  auto stream = ScaleDecoderStream(gsl::make_span(out));
  EXPECT_OUTCOME_TRUE(res, decode<BigInteger>(stream));
  ASSERT_EQ(res, 16384);

  // now only 32768 bytes left in stream
  ASSERT_EQ(stream.hasMore(32768), true);
  ASSERT_EQ(stream.hasMore(32769), false);

  for (auto i = 0; i < length; ++i) {
    EXPECT_OUTCOME_TRUE(byte, decode<uint8_t>(stream))
    ASSERT_EQ(byte, i % 256);
    EXPECT_OUTCOME_TRUE(byte1, decode<uint8_t>(stream))
    ASSERT_EQ(byte1, 0);
  }

  ASSERT_EQ(stream.hasMore(1), false);
}

/**
 * @given very long collection of items of type uint8_t containing 2^20 items
 * this number takes ~ 1 Mb of data
 * where collection[i]  == i % 256
 * @when encodeCollection is applied
 * @then obtain byte array of length 1048576 + 4 bytes (header) bytes
 * where first bytes repreent header, other are data itself
 * where each byte after header == i%256
 */

TEST(Scale, encodeVeryLongCollectionUint8) {
  auto length = 1048576;  // 2^20
  std::vector<uint8_t> collection;
  collection.reserve(length);

  for (auto i = 0; i < length; ++i) {
    collection.push_back(i % 256);
  }

  ScaleEncoderStream s;
  ASSERT_NO_THROW((s << collection));
  auto &&out = s.data();
  ASSERT_EQ(out.size(), (length + 4));
  // header takes 4 bytes,
  // first byte == (4-4) + 3 = 3,
  // which means that number of items requires 4 bytes
  // 3 next bytes are 0, and the last 4-th == 2^6 == 64
  // which is compact-encoded value 2^14 = 16384
  auto stream = ScaleDecoderStream(gsl::make_span(out));
  EXPECT_OUTCOME_TRUE(bi, decode<BigInteger>(stream));
  ASSERT_EQ(bi, 1048576);

  // now only 1048576 bytes left in stream
  ASSERT_EQ(stream.hasMore(1048576), true);
  ASSERT_EQ(stream.hasMore(1048576 + 1), false);

  for (auto i = 0; i < length; ++i) {
    uint8_t byte{0u};
    ASSERT_NO_THROW((stream >> byte));
    ASSERT_EQ(byte, i % 256);
  }

  ASSERT_EQ(stream.hasMore(1), false);
}

// following test takes too much time, don't run it
/**
 * @given very long collection of items of type uint8_t containing 2^30 ==
 * 1073741824 items this number takes ~ 1 Gb of data where collection[i]  == i %
 * 256
 * @when encodeCollection is applied
 * @then obtain byte array of length 1073741824 + 5 bytes (header) bytes
 * where first bytes represent header, other are data itself
 * where each byte after header == i%256
 */
TEST(Scale, DISABLED_encodeVeryLongCollectionUint8) {
  auto length = 1073741824;  // 2^20
  std::vector<uint8_t> collection;

  collection.reserve(length);
  for (auto i = 0; i < length; ++i) {
    collection.push_back(i % 256);
  }

  ScaleEncoderStream s;
  ASSERT_NO_THROW((s << collection));
  auto &&out = s.data();
  ASSERT_EQ(out.size(), (length + 4));
  // header takes 4 bytes,
  // first byte == (4-4) + 3 = 3, which means that number of items
  // requires 4 bytes
  // 3 next bytes are 0, and the last 4-th == 2^6 == 64
  // which is compact-encoded value 2^14 = 16384
  auto stream = ScaleDecoderStream(gsl::make_span(out));
  EXPECT_OUTCOME_TRUE(bi, decode<BigInteger>(stream));
  ASSERT_EQ(bi, length);

  // now only 1048576 bytes left in stream
  ASSERT_EQ(stream.hasMore(length), true);
  ASSERT_EQ(stream.hasMore(length + 1), false);

  for (auto i = 0; i < length; ++i) {
    EXPECT_OUTCOME_TRUE(byte, decode<uint8_t>(stream))
    ASSERT_EQ(byte, i % 256);
  }
  ASSERT_EQ(stream.hasMore(1), false);
}
