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
    stream_.push_back(v);
    return *this;
  }

  ScaleEncoderStream &ScaleEncoderStream::putBuffer(const Buffer &buffer) {
    stream_.insert(stream_.end(), buffer.begin(), buffer.end());
    return *this;
  }

  ScaleEncoderStream &ScaleEncoderStream::put(const std::vector<uint8_t> &v) {
    stream_.insert(stream_.end(), v.begin(), v.end());
    return *this;
  }

  ScaleEncoderStream &ScaleEncoderStream::operator<<(const BigInteger &v) {
    auto &&res = compact::encodeInteger(v, *this);
    if (!res) {
      throw res.error();
    }
    return *this;
  }

  ScaleEncoderStream &ScaleEncoderStream::operator<<(tribool v) {
    auto byte = static_cast<uint8_t>(2);
    if (v) {
      byte = static_cast<uint8_t>(1);
    }
    if (!v) {
      byte = static_cast<uint8_t>(0);
    }
    return putByte(byte);
  }
}  // namespace kagome::scale
