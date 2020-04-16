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

#ifndef KAGOME_CORE_PRIMITIVES_COMMON_HPP
#define KAGOME_CORE_PRIMITIVES_COMMON_HPP

#include <cstdint>

#include <boost/operators.hpp>
#include "primitives/session_key.hpp"

namespace kagome::primitives {
  using BlocksRequestId = uint64_t;

  using BlockNumber = uint64_t;
  using BlockHash = common::Hash256;

  namespace detail {
    // base data structure for the types describing block information
    // (BlockInfo, Prevote, Precommit, PrimaryPropose)
    template <typename Tag>
    struct BlockInfoT : public boost::equality_comparable<BlockInfoT<Tag>> {
      BlockInfoT() = default;

      BlockInfoT(const BlockNumber &n, const BlockHash &h)
          : block_number(n), block_hash(h) {}

      BlockNumber block_number{};
      BlockHash block_hash{};

      bool operator==(const BlockInfoT<Tag> &o) const {
        return block_number == o.block_number && block_hash == o.block_hash;
      }
    };

    template <class Stream,
              typename Tag,
              typename = std::enable_if_t<Stream::is_encoder_stream>>
    Stream &operator<<(Stream &s, const BlockInfoT<Tag> &msg) {
      return s << msg.block_hash << msg.block_number;
    }

    template <class Stream,
              typename Tag,
              typename = std::enable_if_t<Stream::is_decoder_stream>>
    Stream &operator>>(Stream &s, BlockInfoT<Tag> &msg) {
      return s >> msg.block_hash >> msg.block_number;
    }
  }  // namespace detail

  using BlockInfo = detail::BlockInfoT<struct BlockInfoTag>;

}  // namespace kagome::primitives

#endif  // KAGOME_CORE_PRIMITIVES_COMMON_HPP
