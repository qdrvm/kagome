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
   public:
    explicit BufferStream(common::BufferView buf) : data_{buf} {}

    bool hasMore(size_t num_bytes) const {
      return data_.size() >= num_bytes;
    }

    uint8_t next() {
      if (data_.empty()) {
        throw std::out_of_range("Data is out");
      }
      auto byte = data_[0];
      data_.dropFirst(1);
      return byte;
    }

    common::BufferView leftBytes() const {
      return data_;
    }

   private:
    common::BufferView data_;
  };
}  // namespace kagome::storage::trie
