/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "scale/boolean.hpp"

namespace kagome::common::scale::boolean {

  void encodeBool(bool value, Buffer &out) {
    uint8_t byte = (value ? 0x01 : 0x00);
    out.putUint8(byte);
  }

  DecodeBoolResult decodeBool(Stream &stream) {
    auto byte = stream.nextByte();
    if (!byte.has_value()) {
      return expected::Error{DecodeError::kNotEnoughData};
    }

    switch (*byte) {
      case 0:
        return expected::Value{false};
      case 1:
        return expected::Value{true};
      default:
        break;
    }

    return expected::Error{DecodeError::kUnexpectedValue};
  }

  void encodeTribool(tribool value, Buffer &out) {
    auto byte = static_cast<uint8_t>(2);

    if (value) {
      byte = static_cast<uint8_t>(1);
    }

    if (!value) {
      byte = static_cast<uint8_t>(0);
    }
    out.putUint8(byte);
  }

  DecodeTriboolResult decodeTribool(Stream &stream) {
    auto byte = stream.nextByte();
    if (!byte.has_value()) {
      return expected::Error{DecodeError::kNotEnoughData};
    }

    switch (*byte) {
      case 0:
        return expected::Value{false};
      case 1:
        return expected::Value{true};
      case 2:
        return expected::Value{indeterminate};
      default:
        break;
    }

    return expected::Error{DecodeError::kUnexpectedValue};
  }
}  // namespace kagome::common::scale::boolean
