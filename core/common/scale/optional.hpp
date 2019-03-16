/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_OPTIONAL_HPP
#define KAGOME_OPTIONAL_HPP

#include "common/scale/type_decoder.hpp"
#include "common/scale/type_encoder.hpp"
#include "common/scale/types.hpp"

namespace kagome::common::scale::optional {
  /**
   * @brief encodeOptional function encodes optional value
   * @tparam T optional value content type
   * @param optional value
   * @return encoded optional value or error
   */
  template <class T>
  EncodeResult encodeOptional(const std::optional<T> &optional) {
    if (!optional.has_value()) {
      return expected::Value{ByteArray{0}};
    }

    ByteArray result;
    TypeEncoder<T> encoder;
    encoder.encode(*optional).match(
        [&result](const expected::Value<ByteArray> &v) {
          result.push_back(1);  // flag of value presence
          result.insert(result.end(), v.value.begin(), v.value.end());
        },
        [](const expected::Error<EncodeError> &e) {});

    return expected::Value{result};
  }

  /**
   * @brief DecodeOptionalResult type is type of result of decodeOptional
   * function
   * @tparam T optional value content type
   */
  template <class T>
  using DecodeOptionalResult = expected::Result<std::optional<T>, DecodeError>;

  /**
   * @brief decodeOptional function decodes optional value from stream
   * @tparam T optional value content type
   * @param stream source stream
   * @return decoded optional value or error
   */
  template <class T>
  DecodeOptionalResult<T> decodeOptional(Stream &stream) {
    auto flag = stream.nextByte();
    if (!flag.has_value()) {
      return expected::Error{DecodeError::kNotEnoughData};
    }

    if (*flag != 1) {
      return expected::Value{std::nullopt};
    }

    TypeDecoder<T> type_decoder;
    auto r = type_decoder.decode(stream);
    if (!isSucceeded(r)) {
      return expected::Error{DecodeError::kInvalidData};
    }

    T value{};
    r.match([&value](const expected::Value<T> &v) { value = v.value; },
            [](const expected::Error<DecodeError> &) {});

    return expected::Value{std::optional<T>{value}};
  }

  /**
   * @brief specialization of encodeOptional function for optional bool
   * @param optional bool value
   * @return encoded value
   */
  template <>
  EncodeResult encodeOptional(const std::optional<bool> &optional) {
    if (!optional.has_value()) {
      return expected::Value{ByteArray{0}};
    }

    if (!*optional) {
      return expected::Value{ByteArray{1}};
    }

    return expected::Value{ByteArray{2}};
  }

  /**
   * @brief specialization of decodeOptional function for optional bool
   * @param optional bool value
   * @return decoded value or error
   */
  template <>
  DecodeOptionalResult<bool> decodeOptional(Stream &stream) {
    auto byte = stream.nextByte();
    if (!byte.has_value()) {
      return expected::Error{DecodeError::kNotEnoughData};
    }

    switch (*byte) {
      case 0:
        return expected::Value{std::nullopt};
      case 1:
        return expected::Value{std::optional<bool>{false}};
      case 2:
        return expected::Value{std::optional<bool>{true}};
      default:
        break;
    }

    return expected::Error{DecodeError::kUnexpectedValue};
  }

}  // namespace kagome::common::scale::optional

#endif  // KAGOME_OPTIONAL_HPP
