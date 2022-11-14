/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_PRIMITIVES_COMMON_HPP
#define KAGOME_CORE_PRIMITIVES_COMMON_HPP

#include <cstdint>

#include <fmt/format.h>
#include <boost/operators.hpp>

#include "common/blob.hpp"
#include "macro/endianness_utils.hpp"

namespace kagome::primitives {
  using BlockNumber = uint32_t;
  using BlockHash = common::Hash256;
  using ThreadNumber = size_t;

  namespace detail {
    // base data structure for the types describing block information
    // (BlockInfo, Prevote, Precommit, PrimaryPropose)
    template <typename Tag>
    struct BlockInfoT : public boost::equality_comparable<BlockInfoT<Tag>>,
                        public boost::less_than_comparable<BlockInfoT<Tag>> {
      BlockInfoT() = default;

      BlockInfoT(const BlockNumber &n, const BlockHash &h)
          : number(n), hash(h) {}

      BlockInfoT(const BlockHash &h, const BlockNumber &n)
          : number(n), hash(h) {}

      BlockNumber number{};
      BlockHash hash{};

      bool operator==(const BlockInfoT<Tag> &o) const {
        return number == o.number && hash == o.hash;
      }

      bool operator<(const BlockInfoT<Tag> &o) const {
        return number < o.number or (number == o.number and hash < o.hash);
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

template <typename Tag>
struct fmt::formatter<kagome::primitives::detail::BlockInfoT<Tag>> {
  // Presentation format: 's' - short, 'l' - long.
  char presentation = 's';

  // Parses format specifications of the form ['s' | 'l'].
  constexpr auto parse(format_parse_context &ctx) -> decltype(ctx.begin()) {
    // Parse the presentation format and store it in the formatter:
    auto it = ctx.begin(), end = ctx.end();
    if (it != end && (*it == 's' || *it == 'l')) {
      presentation = *it++;
    }

    // Check if reached the end of the range:
    if (it != end && *it != '}') {
      throw format_error("invalid format");
    }

    // Return an iterator past the end of the parsed range:
    return it;
  }

  // Formats the BlockInfo using the parsed format specification (presentation)
  // stored in this formatter.
  template <typename FormatContext>
  auto format(const kagome::primitives::detail::BlockInfoT<Tag> &block_info,
              FormatContext &ctx) const -> decltype(ctx.out()) {
    // ctx.out() is an output iterator to write to.

    if (presentation == 's') {
      static_assert(decltype(block_info.hash)::size() > 4);
      return format_to(
          ctx.out(),
          "#{} (0x{:04x}…{:04x})",
          block_info.number,
          // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
          htobe16(*reinterpret_cast<const uint16_t *>(block_info.hash.data())),
          // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
          htobe16(*reinterpret_cast<const uint16_t *>(block_info.hash.data()
                                                      + block_info.hash.size()
                                                      - sizeof(uint16_t))));
    }

    return format_to(
        ctx.out(), "#{} (0x{})", block_info.number, block_info.hash.toHex());
  }
};

#endif  // KAGOME_CORE_PRIMITIVES_COMMON_HPP
