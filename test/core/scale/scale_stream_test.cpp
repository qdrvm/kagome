/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "scale/scale_encoder_stream.hpp"

#include <iostream>

#include <gtest/gtest.h>
#include "scale/byte_array_stream.hpp"
#include "scale/type_decoder.hpp"
#include "scale/type_encoder.hpp"

using kagome::common::Buffer;
using kagome::common::ByteStream;
using kagome::scale::ByteArray;
using kagome::scale::ByteArrayStream;
using kagome::scale::ScaleEncoderStream;

std::string stringify_buffer(const Buffer &buffer) {
  std::string result;
  for (auto & item : buffer) {
    result += std::to_string(item) + " ";
  }
  return result;
}

TEST(ScaleStreamTest, Simple) {
  ScaleEncoderStream s;
  s << static_cast<uint32_t>(1u) << static_cast<uint64_t>(1u)
    << static_cast<int64_t>(-1) << 1ul;
  std::cout << "[   content   ] " << stringify_buffer(s.getBuffer()) << std::endl;
}

TEST(ScaleStreamTest, EncodeCollection) {
  std::vector<uint16_t> v = {1u, 2u, 3u, 4u};
  ScaleEncoderStream s;
  s << v;
  std::cout << "[  collection  ] " << stringify_buffer(s.getBuffer()) << std::endl;
}

TEST(ScaleStreamTest, EncodePair) {
  std::pair<uint8_t, uint32_t> v = {1u, 2u};
  ScaleEncoderStream s;
  s << v;
  std::cout << "[  pair  ] " << stringify_buffer(s.getBuffer()) << std::endl;
}

TEST(ScaleStreamTest, EncodeOptional) {
  std::optional<uint32_t> v1 = std::nullopt;
  std::optional <uint32_t> v2 = 257ul;
  ScaleEncoderStream s;
  (s << v1).putByte(-1) << v2;
  std::cout << "[ optional ] " << stringify_buffer(s.getBuffer()) << std::endl;
}
