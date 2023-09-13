/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <fmt/format.h>
#include <libp2p/peer/peer_id.hpp>
#include <string_view>

// Remove after it will be added to libp2p (will be happened compilation error)
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

  // Formats the PeerId using the parsed format specification (presentation)
  // stored in this formatter.
  template <typename FormatContext>
  auto format(const libp2p::peer::PeerId &peer_id, FormatContext &ctx) const
      -> decltype(ctx.out()) {
    // ctx.out() is an output iterator to write to.

    auto &&b58 = peer_id.toBase58();

    if (presentation == 's') {
      static constexpr string_view message("…");
      auto out = std::copy(message.begin(), message.end(), ctx.out());
      return std::copy_n(
          b58.begin() + (b58.size() - std::min<size_t>(6, b58.size())),
          std::min<size_t>(6, b58.size()),
          out);
    }

    return std::copy(std::begin(b58), std::end(b58), ctx.out());
  }
};
