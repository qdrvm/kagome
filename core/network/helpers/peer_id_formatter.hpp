/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <fmt/format.h>
#include <libp2p/peer/peer_id.hpp>
#include <string_view>

template <>
struct fmt::formatter<libp2p::peer::PeerId> {
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

  // Formats the Blob using the parsed format specification (presentation)
  // stored in this formatter.
  template <typename FormatContext>
  auto format(const libp2p::peer::PeerId &peer_id, FormatContext &ctx)
      -> decltype(ctx.out()) {
    // ctx.out() is an output iterator to write to.

    auto &&b58 = peer_id.toBase58();

    if (presentation == 's') {
      return format_to(ctx.out(),
                       "…{}",
                       std::string_view(b58.data() + b58.size()
                                            - std::min<size_t>(6, b58.size()),
                                        std::min<size_t>(6, b58.size())));
    }

    return format_to(ctx.out(), "{}", b58);
  }
};
