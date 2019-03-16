/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "common/result.hpp"
#include "common/scale/basic_stream.hpp"
#include "common/scale/optional.hpp"

using namespace kagome;
using namespace kagome::common;
using namespace common::scale;

TEST(Scale, encodeOptional) {
  // most simple case
  optional::encodeOptional(std::optional<uint8_t>{std::nullopt})
      .match(
          [](const expected::Value<ByteArray> &v) {
            ASSERT_EQ(v.value, (ByteArray{0}));
          },
          [](const expected::Error<EncodeError> &) { FAIL(); });

  // encode existing uint8_t
  optional::encodeOptional(std::optional<uint8_t>{1})
      .match(
          [](const expected::Value<ByteArray> &v) {
            ASSERT_EQ(v.value, (ByteArray{1, 1}));
          },
          [](const expected::Error<EncodeError> &) { FAIL(); });
  // encode negative int8_t
  optional::encodeOptional(std::optional<int8_t>{-1})
      .match(
          [](const expected::Value<ByteArray> &v) {
            ASSERT_EQ(v.value, (ByteArray{1, 255}));
          },
          [](const expected::Error<EncodeError> &) { FAIL(); });

  // encode non-existing uint16_t
  optional::encodeOptional<uint16_t>(std::nullopt)
      .match(
          [](const expected::Value<ByteArray> &v) {
            ASSERT_EQ(v.value, (ByteArray{0}));
          },
          [](const expected::Error<EncodeError> &) { FAIL(); });

  // encode existing uint16_t
  optional::encodeOptional(std::optional<uint16_t>{511})
      .match(
          [](const expected::Value<ByteArray> &v) {
            ASSERT_EQ(v.value, (ByteArray{1, 255, 1}));
          },
          [](const expected::Error<EncodeError> &) { FAIL(); });

  // encode existing uint32_t
  optional::encodeOptional(std::optional<uint32_t>{67305985})
      .match(
          [](const expected::Value<ByteArray> &v) {
            ASSERT_EQ(v.value, (ByteArray{1, 1, 2, 3, 4}));
          },
          [](const expected::Error<EncodeError> &) { FAIL(); });
}

TEST(Scale, decodeOptional) {
  // clang-format off
    auto bytes = ByteArray{
            0,              // first value
            1, 1,           // second value
            1, 255,         // third value
            0,              // fourth value
            1, 255, 1,      // fifth value
            1, 1, 2, 3, 4}; // sixth value
  // clang-format on

  auto stream = BasicStream{bytes};

  // decode nullopt uint8_t
  optional::decodeOptional<uint8_t>(stream).match(
      [](const expected::Value<std::optional<uint8_t>> &v) {
        ASSERT_EQ(v.value, std::nullopt);
      },
      [](const expected::Error<DecodeError> &) { FAIL(); });

  // decode optional uint8_t
  optional::decodeOptional<uint8_t>(stream).match(
      [](const expected::Value<std::optional<uint8_t>> &v) {
        ASSERT_EQ(v.value.has_value(), true);
        ASSERT_EQ((*v.value), 1);
      },
      [](const expected::Error<DecodeError> &) { FAIL(); });

  // decode optional negative int8_t
  optional::decodeOptional<int8_t>(stream).match(
      [](const expected::Value<std::optional<int8_t>> &v) {
        ASSERT_EQ(v.value.has_value(), true);
        ASSERT_EQ((*v.value), -1);
      },
      [](const expected::Error<DecodeError> &) { FAIL(); });

  // decode nullopt uint16_t
  // it requires 1 zero byte just like any other nullopt
  optional::decodeOptional<uint16_t>(stream).match(
      [](const expected::Value<std::optional<uint16_t>> &v) {
        ASSERT_EQ(v.value.has_value(), false);
      },
      [](const expected::Error<DecodeError> &) { FAIL(); });

  // decode optional uint16_t
  optional::decodeOptional<uint16_t>(stream).match(
      [](const expected::Value<std::optional<uint16_t>> &v) {
        ASSERT_EQ(v.value.has_value(), true);
        ASSERT_EQ((*v.value), 511);
      },
      [](const expected::Error<DecodeError> &) { FAIL(); });

  // decode optional uint32_t
  optional::decodeOptional<uint32_t>(stream).match(
      [](const expected::Value<std::optional<uint32_t>> &v) {
        ASSERT_EQ(v.value.has_value(), true);
        ASSERT_EQ((*v.value), 67305985);
      },
      [](const expected::Error<DecodeError> &) { FAIL(); });
}