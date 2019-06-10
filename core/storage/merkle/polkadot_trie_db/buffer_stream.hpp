/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_STORAGE_MERKLE_POLKADOT_TRIE_DB_BUFFER_STREAM_HPP_
#define KAGOME_CORE_STORAGE_MERKLE_POLKADOT_TRIE_DB_BUFFER_STREAM_HPP_

#include <gsl/span>
#include "common/buffer.hpp"

namespace kagome::storage::merkle {

  /**
   * Needed for decoding, might be replaced to a more common stream in the
   * future, when one appears
   */
  class BufferStream {
   public:
    explicit BufferStream(const common::Buffer &buf) : data_{buf.toVector()} {}

    bool hasMore(size_t num_bytes) const {
      return data_.size() >= num_bytes;
    }

    uint8_t next() {
      auto byte = data_.at(0);
      data_ = data_.last(data_.size() - 1);
      return byte;
    }

    gsl::span<const uint8_t> leftBytes() const {
      return data_;
    }

   private:
    gsl::span<const uint8_t> data_;
  };
}  // namespace kagome::storage::merkle
#endif  // KAGOME_CORE_STORAGE_MERKLE_POLKADOT_TRIE_DB_BUFFER_STREAM_HPP_
