/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "scale/scale_encoder_stream.hpp"

namespace kagome::scale {
  using common::Buffer;

  Buffer ScaleEncoderStream::getBuffer() const {
    Buffer buffer(stream_.size(), 0u);
    for (auto [it, dest] = std::pair(stream_.begin(), buffer.begin());
         it != stream_.end(); ++it, ++dest) {
      *dest = *it;
    }
    return buffer;
  }

  ScaleEncoderStream &ScaleEncoderStream::putByte(uint8_t v) {
    return *this << static_cast<uint8_t>(v);
  }

  ScaleEncoderStream &ScaleEncoderStream::putBuffer(const Buffer &buffer) {
    stream_.insert(stream_.end(), buffer.begin(), buffer.end());
    return *this;
  }

  ScaleEncoderStream &ScaleEncoderStream::put(const std::vector<uint8_t> &v) {
    stream_.insert(stream_.end(), v.begin(), v.end());
    return *this;
  }

  ScaleEncoderStream &ScaleEncoderStream::operator<<(uint8_t v) {
    fixedwidth::encodeUint8(v, *this);
    return *this;
  }

  ScaleEncoderStream &ScaleEncoderStream::operator<<(int8_t v) {
    fixedwidth::encodeInt8(v, *this);
    return *this;
  }

  ScaleEncoderStream &ScaleEncoderStream::operator<<(uint16_t v) {
    fixedwidth::encodeUint16(v, *this);
    return *this;
  }

  ScaleEncoderStream &ScaleEncoderStream::operator<<(int16_t v) {
    fixedwidth::encodeInt16(v, *this);
    return *this;
  }

  ScaleEncoderStream &ScaleEncoderStream::operator<<(uint32_t v) {
    fixedwidth::encodeUint32(v, *this);
    return *this;
  }

  ScaleEncoderStream &ScaleEncoderStream::operator<<(int32_t v) {
    fixedwidth::encodeInt32(v, *this);
    return *this;
  }

  ScaleEncoderStream &ScaleEncoderStream::operator<<(uint64_t v) {
    fixedwidth::encodeUint64(v, *this);
    return *this;
  }

  ScaleEncoderStream &ScaleEncoderStream::operator<<(int64_t v) {
    fixedwidth::encodeInt64(v, *this);
    return *this;
  }

  ScaleEncoderStream &ScaleEncoderStream::operator<<(unsigned long v) {
    switch (sizeof(unsigned long)) {
      case 4:
        return *this << static_cast<uint32_t>(v);
      case 8:
        return *this << static_cast<uint64_t>(v);
      default:
        std::terminate();
    }
  }

  ScaleEncoderStream &ScaleEncoderStream::operator<<(const BigInteger &v) {
    auto &&res = compact::encodeInteger(v, *this);
    if (!res) {
      throw res.error();
    }
    return *this;
  }

  ScaleEncoderStream &ScaleEncoderStream::operator<<(bool v) {
    uint8_t byte = (v ? 0x01u : 0x00u);
    return putByte(byte);
  }
}  // namespace kagome::scale
