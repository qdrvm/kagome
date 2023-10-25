/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <span>

#include "common/buffer.hpp"

namespace kagome::storage::trie {

  /**
   * Needed for decoding, might be replaced to a more common stream in the
   * future, when one appears
   */
  class BufferStream {
    using index_type = std::span<const uint8_t>::size_type;

   public:
    explicit BufferStream(std::span<const uint8_t> buf) : data_{buf} {}

    bool hasMore(index_type num_bytes) const {
      return data_.size() >= num_bytes;
    }

    uint8_t next() {
      if (data_.empty()) {
        throw std::out_of_range("Data is out");
      }
      auto byte = data_[0];
      data_ = data_.last(data_.size() - 1);
      return byte;
    }

    std::span<const uint8_t> leftBytes() const {
      return data_;
    }

   private:
    std::span<const uint8_t> data_;
  };
}  // namespace kagome::storage::trie
