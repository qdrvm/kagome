/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "common/result.hpp"
#include "common/scale/basic_stream.hpp"
#include "common/scale/collection.hpp"

using namespace kagome;
using namespace kagome::common;
using namespace common::scale;

/**
 * @given collection of 80 items of type uint8_t
 * @when encodeCollection is applied
 * @then expected result is obtained: header is 2 byte, items are 1 byte each
 */
TEST(Scale, encodeCollectionOf80) {
  // 80 items of value 1
  std::vector<uint8_t> collection = {
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  };
  auto res = collection::encodeCollection(collection);
  res.match(
      [&collection](const EncodeResult::ValueType &v) {
        // clang-format off
                ASSERT_EQ(v.value,
                          (ByteArray{65, 1, // header: 80 > 63 so 2 bytes required
                                  // (80 << 2) + 1 = 321 = 65 + 256 * 1
                                     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                                     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                                     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                                     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                          }));
        // clang-format on
      },
      [](const EncodeResult::ErrorType &e) { FAIL(); });
}

/**
 * @given collection of items of type uint16_t
 * @when encodeCollection is applied
 * @then expected result is obtained
 */
TEST(Scale, encodeCollectionUint16) {
  std::vector<uint16_t> collection = {1, 2, 3, 4};
  auto res = collection::encodeCollection(collection);
  res.match(
      [&collection](const EncodeResult::ValueType &v) {
        // clang-format off
                ASSERT_EQ(v.value,
                          (ByteArray{
                                  16,  // header
                                  1, 0,  // first item
                                  2, 0,  // second item
                                  3, 0,  // third item
                                  4, 0  // fourth item
                          }));
        // clang-format on
      },
      [](const EncodeResult::ErrorType &e) { FAIL(); });
}
// check the same for 4 and 8 bytes

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

  auto res = collection::encodeCollection(collection);
  res.match(
      [&collection, length](const EncodeResult::ValueType &v) {
        ASSERT_EQ(v.value.size(), (length * 2 + 4));
        // header takes 4 byte,
        // first 4 bytes represent le-encoded value 2^16 + 2
        // which is compact-encoded value 2^14 = 16384
        auto stream = BasicStream(v.value);

        compact::decodeInteger(stream).match(
            [](const compact::DecodeIntegerResult::ValueType &bi) {
              ASSERT_EQ(bi.value, 16384);
            },
            [](const compact::DecodeIntegerResult::ErrorType &) { FAIL(); });
        // now only 32768 bytes left in stream
        ASSERT_EQ(stream.hasMore(32768), true);
        ASSERT_EQ(stream.hasMore(32769), false);

        for (auto i = 0; i < length; ++i) {
          auto byte = stream.nextByte();
          ASSERT_EQ(byte.has_value(), true);
          ASSERT_EQ((*byte), i % 256);
          byte = stream.nextByte();
          ASSERT_EQ(byte.has_value(), true);
          ASSERT_EQ((*byte), 0);
        }

        ASSERT_EQ(stream.hasMore(1), false);
      },
      [](const EncodeResult::ErrorType &e) { FAIL(); });
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
  std::vector<uint8_t> collection;
  auto length = 1048576;  // 2^20
  collection.reserve(length);
  for (auto i = 0; i < length; ++i) {
    collection.push_back(i % 256);
  }

  auto res = collection::encodeCollection(collection);
  res.match(
      [&collection, length](const EncodeResult::ValueType &v) {
        ASSERT_EQ(v.value.size(), (length + 4));
        // header takes 4 bytes,
        // first byte == (4-4) + 3 = 3, which means that number of items
        // requires 4 bytes
        // 3 next bytes are 0, and the last 4-th == 2^6 == 64
        // which is compact-encoded value 2^14 = 16384
        auto stream = BasicStream(v.value);

        compact::decodeInteger(stream).match(
            [](const compact::DecodeIntegerResult::ValueType &bi) {
              ASSERT_EQ(bi.value, 1048576);
            },
            [](const compact::DecodeIntegerResult::ErrorType &) { FAIL(); });
        // now only 1048576 bytes left in stream
        ASSERT_EQ(stream.hasMore(1048576), true);
        ASSERT_EQ(stream.hasMore(1048576 + 1), false);

        for (auto i = 0; i < length; ++i) {
          auto byte = stream.nextByte();
          ASSERT_EQ(byte.has_value(), true);
          ASSERT_EQ((*byte), i % 256);
        }

        ASSERT_EQ(stream.hasMore(1), false);
      },
      [](const EncodeResult::ErrorType &e) { FAIL(); });
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
  std::vector<uint8_t> collection;
  auto length = 1073741824;  // 2^20
  collection.reserve(length);
  for (auto i = 0; i < length; ++i) {
    collection.push_back(i % 256);
  }

  auto res = collection::encodeCollection(collection);
  res.match(
      [&collection, length](const EncodeResult::ValueType &v) {
        ASSERT_EQ(v.value.size(), (length + 4));
        // header takes 4 bytes,
        // first byte == (4-4) + 3 = 3, which means that number of items
        // requires 4 bytes
        // 3 next bytes are 0, and the last 4-th == 2^6 == 64
        // which is compact-encoded value 2^14 = 16384
        auto stream = BasicStream(v.value);

        compact::decodeInteger(stream).match(
            [length](const compact::DecodeIntegerResult::ValueType &bi) {
              ASSERT_EQ(bi.value, length);
            },
            [](const compact::DecodeIntegerResult::ErrorType &) { FAIL(); });
        // now only 1048576 bytes left in stream
        ASSERT_EQ(stream.hasMore(length), true);
        ASSERT_EQ(stream.hasMore(length + 1), false);

        for (auto i = 0; i < length; ++i) {
          auto byte = stream.nextByte();
          ASSERT_EQ(byte.has_value(), true);
          ASSERT_EQ((*byte), i % 256);
        }

        ASSERT_EQ(stream.hasMore(1), false);
      },
      [](const EncodeResult::ErrorType &e) { FAIL(); });
}

/**
 * @given byte array representing encoded collection of
 * 4 uint16_t numbers {1, 2, 3, 4}
 * @when decodeCollection is applied
 * @then decoded collection {1, 2, 3, 4} is obtained
 */
TEST(Scale, decodeSimpleCollectionOfUint16) {
  std::vector<uint16_t> collection = {1, 2, 3, 4};
  // clang-format off
    auto bytes = ByteArray{
            16,   // header
            1, 0, // first item
            2, 0, // second item
            3, 0, // third item
            4, 0  // fourth item
    };
  // clang-format on
  auto stream = BasicStream{bytes};
  auto res = collection::decodeCollection<uint16_t>(stream);
  res.match(
      [&collection](const expected::Value<std::vector<uint16_t>> &v) {
        ASSERT_EQ(v.value.size(), 4);
        ASSERT_EQ(v.value, collection);
      },
      [](const expected::Error<DecodeError> &e) { FAIL(); });
}

/**
 * @given encoded long collection ~ 1 Mb of data
 * @when apply decodeCollection
 * @then obtain source collection
 */
TEST(Scale, decodeLongCollectionOfUint8) {
  std::vector<uint8_t> collection;
  auto length = 1048576;  // 2^20
  collection.reserve(length);
  for (auto i = 0; i < length; ++i) {
    collection.push_back(i % 256);
  }

  auto res = collection::encodeCollection(collection);
  res.match(
      [&collection, length](const EncodeResult::ValueType &v) {
        auto stream = BasicStream(v.value);
        auto decodeResult = collection::decodeCollection<uint8_t>(stream);
        decodeResult.match(
            [&collection](
                collection::DecodeCollectionResult<uint8_t>::ValueType &val) {
              ASSERT_EQ(val.value, collection);
            },
            [](expected::Error<DecodeError> &) { FAIL(); });
      },
      [](const EncodeResult::ErrorType &e) { FAIL(); });
}
