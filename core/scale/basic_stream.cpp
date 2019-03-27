/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "scale/basic_stream.hpp"
#include "basic_stream.hpp"

namespace kagome::common::scale {

  std::optional<uint8_t> BasicStream::nextByte() {
    if (!hasMore(1)) {
      return std::nullopt;
    }

    --bytes_left_;
    return *current_iterator_++;
  }

  BasicStream::BasicStream(const ByteArray &source)
      : current_iterator_{source.begin()},
        bytes_left_{static_cast<int64_t>(source.size())} {}

  BasicStream::BasicStream(const Buffer &source)
      : current_iterator_{source.begin()},
        bytes_left_{static_cast<int64_t>(source.size())} {}

  bool BasicStream::hasMore(uint64_t n) const {
    return bytes_left_ >= n;
  }
}  // namespace kagome::common::scale
