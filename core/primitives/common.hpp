/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_PRIMITIVES_COMMON_HPP
#define KAGOME_CORE_PRIMITIVES_COMMON_HPP

#include <cstdint>

#include <boost/operators.hpp>

#include "common/blob.hpp"

namespace kagome::primitives {
  using BlocksRequestId = uint64_t;

  using BlockNumber = uint32_t;
  using BlockHash = common::Hash256;

  namespace detail {
    // base data structure for the types describing block information
    // (BlockInfo, Prevote, Precommit, PrimaryPropose)
    template <typename Tag>
    struct BlockInfoT : public boost::equality_comparable<BlockInfoT<Tag>> {
      BlockInfoT() = default;

      BlockInfoT(const BlockNumber &n, const BlockHash &h)
          : number(n), hash(h) {}

      BlockNumber number{};
      BlockHash hash{};

      bool operator==(const BlockInfoT<Tag> &o) const {
        return number == o.number && hash == o.hash;
      }
    };

    template <class Stream,
              typename Tag,
              typename = std::enable_if_t<Stream::is_encoder_stream>>
    Stream &operator<<(Stream &s, const BlockInfoT<Tag> &msg) {
      return s << msg.hash << msg.number;
    }

    template <class Stream,
              typename Tag,
              typename = std::enable_if_t<Stream::is_decoder_stream>>
    Stream &operator>>(Stream &s, BlockInfoT<Tag> &msg) {
      return s >> msg.hash >> msg.number;
    }
  }  // namespace detail

  using BlockInfo = detail::BlockInfoT<struct BlockInfoTag>;

}  // namespace kagome::primitives

#endif  // KAGOME_CORE_PRIMITIVES_COMMON_HPP
