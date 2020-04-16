/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef KAGOME_CORE_STORAGE_TRIE_POLKADOT_TRIE_DB_BUFFER_STREAM_HPP
#define KAGOME_CORE_STORAGE_TRIE_POLKADOT_TRIE_DB_BUFFER_STREAM_HPP

#include <gsl/span>
#include "common/buffer.hpp"

namespace kagome::storage::trie {

  /**
   * Needed for decoding, might be replaced to a more common stream in the
   * future, when one appears
   */
  class BufferStream {
      using index_type = gsl::span<const uint8_t>::index_type;

   public:
    explicit BufferStream(const common::Buffer &buf) : data_{buf.toVector()} {}

    bool hasMore(index_type num_bytes) const {
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
}  // namespace kagome::storage::trie
#endif  // KAGOME_CORE_STORAGE_TRIE_POLKADOT_TRIE_DB_BUFFER_STREAM_HPP
