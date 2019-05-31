/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "scale/byte_array_stream.hpp"
#include "scale/compact.hpp"
#include "scale/scale.hpp"
#include "scale/scale_decoder_stream.hpp"
#include "scale/scale_encoder_stream.hpp"
#include "testutil/outcome.hpp"

using kagome::scale::ByteArray;
using kagome::scale::ByteArrayStream;
using kagome::scale::encode;
using kagome::scale::ScaleEncoderStream;

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
  // clang-format off
  ASSERT_EQ(out, match);
  // clang-format on
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
  auto stream = ByteArrayStream(out);

  auto &&res1 = kagome::scale::compact::decodeInteger(stream);
  ASSERT_TRUE(res1);
  ASSERT_EQ(res1.value(), 16384);

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
  auto stream = ByteArrayStream(out);

  auto &&bi = kagome::scale::compact::decodeInteger(stream);
  ASSERT_TRUE(bi);
  ASSERT_EQ(bi.value(), 1048576);

  // now only 1048576 bytes left in stream
  ASSERT_EQ(stream.hasMore(1048576), true);
  ASSERT_EQ(stream.hasMore(1048576 + 1), false);

  for (auto i = 0; i < length; ++i) {
    auto byte = stream.nextByte();
    ASSERT_EQ(byte.has_value(), true);
    ASSERT_EQ((*byte), i % 256);
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
  auto stream = ByteArrayStream(out);

  auto &&bi = kagome::scale::compact::decodeInteger(stream);
  ASSERT_TRUE(bi);
  ASSERT_EQ(bi.value(), length);

  // now only 1048576 bytes left in stream
  ASSERT_EQ(stream.hasMore(length), true);
  ASSERT_EQ(stream.hasMore(length + 1), false);

  for (auto i = 0; i < length; ++i) {
    auto byte = stream.nextByte();
    ASSERT_EQ(byte.has_value(), true);
    ASSERT_EQ((*byte), i % 256);
  }

  ASSERT_EQ(stream.hasMore(1), false);
}

///**
// * @given byte array representing encoded collection of
// * 4 uint16_t numbers {1, 2, 3, 4}
// * @when decodeCollection is applied
// * @then decoded collection {1, 2, 3, 4} is obtained
// */
// TEST(Scale, decodeSimpleCollectionOfUint16) {
//  std::vector<uint16_t> collection = {1, 2, 3, 4};
//  // clang-format off
//    auto bytes = ByteArray{
//            16,   // header
//            1, 0, // first item
//            2, 0, // second item
//            3, 0, // third item
//            4, 0  // fourth item
//    };
//  // clang-format on
//  auto stream = ByteArrayStream{bytes};
//  auto &&res = kagome::scale::collection::decodeCollection<uint16_t>(stream);
//  ASSERT_TRUE(res);
//  auto &&value = res.value();
//  ASSERT_EQ(value.size(), 4);
//  ASSERT_EQ(value, collection);
//}
//
///**
// * @given encoded long collection ~ 1 Mb of data
// * @when apply decodeCollection
// * @then obtain source collection
// */
// TEST(Scale, decodeLongCollectionOfUint8) {
//  std::vector<uint8_t> collection;
//  auto length = 1048576;  // 2^20
//  collection.reserve(length);
//  for (auto i = 0; i < length; ++i) {
//    collection.push_back(i % 256);
//  }
//
//  EXPECT_OUTCOME_TRUE(out, encode(collection))
//  auto stream = ByteArrayStream(out);
//  EXPECT_OUTCOME_TRUE(
//      decode_result,
//      kagome::scale::collection::decodeCollection<uint8_t>(stream))
//  ASSERT_EQ(decode_result, collection);
//}
//
///**
// * @given byte array representing encoded collection of
// * 4 uint16_t numbers {50462976, 117835012, 185207048, 252579084}
// * @when decodeCollection is applied
// * @then decoded collection is obtained
// */
// TEST(Scale, decodeSimpleCollectionOfUint32) {
//  // clang-format off
//  std::vector<uint32_t>  collection = {
//          50462976, 117835012, 185207048, 252579084};
//
//  auto bytes = ByteArray{
//          16,                // header
//          0, 1, 2, 3,        // first item
//          4, 5, 6, 7,        // second item
//          8, 9, 0xA, 0xB,    // third item
//          0xC, 0xD, 0xE, 0xF // fourth item
//  };
//  // clang-format on
//  auto stream = ByteArrayStream{bytes};
//  auto &&res = kagome::scale::collection::decodeCollection<uint32_t>(stream);
//  ASSERT_TRUE(res);
//  auto &&val = res.value();
//  ASSERT_EQ(val.size(), 4);
//  ASSERT_EQ(val, collection);
//}
//
///**
// * @given byte array representing encoded collection of
// * two uint16_t numbers {506097522914230528ull, 1084818905618843912ull}
// * @when decodeCollection is applied
// * @then decoded collection is obtained
// */
// TEST(Scale, decodeSimpleCollectionOfUint64) {
//  // clang-format off
//  std::vector<uint64_t>  collection = {
//          506097522914230528ull,
//          1084818905618843912ull};
//
//  auto bytes = ByteArray{
//          8,                // header
//          0, 1, 2, 3,        // first item
//          4, 5, 6, 7,        // second item
//          8, 9, 0xA, 0xB,    // third item
//          0xC, 0xD, 0xE, 0xF // fourth item
//  };
//  // clang-format on
//  auto stream = ByteArrayStream{bytes};
//  auto &&res = kagome::scale::collection::decodeCollection<uint64_t>(stream);
//  ASSERT_TRUE(res);
//  auto &&val = res.value();
//  ASSERT_EQ(val.size(), 2);
//  ASSERT_EQ(val, collection);
//}
