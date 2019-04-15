/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SCALE_BASIC_STREAM_HPP
#define KAGOME_SCALE_BASIC_STREAM_HPP

#include <optional>

#include "common/buffer.hpp"
#include "common/byte_stream.hpp"
#include "scale/types.hpp"

namespace kagome::scale {
  /**
   * @class ByteArrayStream implements Stream interface
   * It wraps ByteArray and allows getting bytes
   * from it sequentially.
   */
  class ByteArrayStream : public common::ByteStream {
   public:
    explicit ByteArrayStream(const ByteArray &source);

    explicit ByteArrayStream(const common::Buffer &source);

    ~ByteArrayStream() override = default;

    bool hasMore(uint64_t n) const override;

    std::optional<uint8_t> nextByte() override;

    outcome::result<void> advance(uint64_t dist) override;

   private:
    ByteArray::const_iterator current_iterator_;  ///< current byte iterator
    int64_t bytes_left_;                          ///< remaining bytes counter
  };
}  // namespace kagome::scale

#endif  // KAGOME_SCALE_BASIC_STREAM_HPP
