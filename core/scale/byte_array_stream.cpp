/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "scale/byte_array_stream.hpp"

namespace kagome::scale {

  ByteArrayStream::ByteArrayStream(const ByteArray &source)
      : current_iterator_{source.begin()},
        bytes_left_{static_cast<int64_t>(source.size())} {}

  ByteArrayStream::ByteArrayStream(const common::Buffer &source)
      : current_iterator_{source.begin()},
        bytes_left_{static_cast<int64_t>(source.size())} {}

  std::optional<uint8_t> ByteArrayStream::nextByte() {
    if (!hasMore(1)) {
      return std::nullopt;
    }

    --bytes_left_;
    return *current_iterator_++;
  }

  bool ByteArrayStream::hasMore(uint64_t n) const {
    return bytes_left_ >= n;
  }

  outcome::result<void> ByteArrayStream::advance(uint64_t dist) {
    if (not hasMore(dist)) {
      return AdvanceErrc::OUT_OF_BOUNDARIES;
    }
    std::advance(current_iterator_, dist);
    bytes_left_ -= dist;
    return outcome::success();
  }
}  // namespace kagome::scale
