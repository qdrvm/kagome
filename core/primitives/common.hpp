/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
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

  using AuthorityId = SessionKey;
  using AuthorityIndex = uint64_t;

  namespace detail {
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

    // TODO(akvinikym) 04.09.19: implement the SCALE codecs
    template <class Stream,
              typename Tag,
              typename = std::enable_if_t<Stream::is_encoder_stream>>
    Stream &operator<<(Stream &s, const BlockInfoT<Tag> &msg) {
      return s;
    }

    template <class Stream,
              typename Tag,
              typename = std::enable_if_t<Stream::is_decoder_stream>>
    Stream &operator>>(Stream &s, const BlockInfoT<Tag> &msg) {
      return s;
    }
  }  // namespace detail

  using BlockInfo = detail::BlockInfoT<struct BlockInfoTag>;

}  // namespace kagome::primitives

#endif  // KAGOME_CORE_PRIMITIVES_COMMON_HPP
