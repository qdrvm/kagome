/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_OPTIONAL_HPP
#define KAGOME_OPTIONAL_HPP

#include "common/buffer.hpp"
#include "scale/type_decoder.hpp"
#include "scale/type_encoder.hpp"
#include "scale/types.hpp"

namespace kagome::common::scale::optional {
  /**
   * @brief encodeOptional function encodes optional value
   * @tparam T optional value content type
   * @param optional value
   * @return encoded optional value or error
   */
  template <class T>
  bool encodeOptional(const std::optional<T> &optional, Buffer &out) {
    if (!optional.has_value()) {
      out.putUint8(0);
      return true;
    }

    Buffer tmp;
    auto type_encode_result = TypeEncoder<T>{}.encode(*optional, tmp);
    if (type_encode_result != true) {
      return type_encode_result;
    }

    out.putUint8(1);
    out.put(tmp.toVector());

    return true;
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
  bool encodeOptional<bool>(const std::optional<bool> &optional, Buffer &out) {
    uint8_t result = 2;  // true

    if (!optional.has_value()) {  // none
      result = 0;
    } else if (!*optional) {  // false
      result = 1;
    }

    out.putUint8(result);
    return true;
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
