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

    auto typeEncodeResult = TypeEncoder<T>{}.encode(*optional);
    if (typeEncodeResult.hasError()) {
      return typeEncodeResult;
    }

    ByteArray result = {1};
    auto &value_ref = typeEncodeResult.getValueRef();
    result.insert(result.end(), value_ref.begin(), value_ref.end());

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

    auto result = TypeDecoder<T>{}.decode(stream);
    if (result.hasError()) {
      return expected::Error{result.getError()};
    }

    return expected::Value{std::optional<T>{std::move(result.getValueRef())}};
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
