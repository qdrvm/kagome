/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gsl/span>

#include "common/bytestr.hpp"
#include "common/hexutil.hpp"
#include "common/lexicographical_compare_three_way.hpp"
#include "macro/endianness_utils.hpp"

namespace kagome::common {
  template <size_t MaxSize>
  class SLBuffer;
}

namespace kagome::common {

  class BufferView : public gsl::span<const uint8_t> {
    using Span = gsl::span<const uint8_t>;

   public:
    using Span::Span;
    using Span::operator=;

    BufferView(const Span &other) noexcept : Span(other) {}

    std::string toHex() const {
      return hex_lower(*this);
    }

    std::string_view toStringView() const {
      return byte2str(*this);
    }
  };

  template <typename L, typename R>
    requires(std::is_base_of_v<BufferView, L>
             or std::is_base_of_v<BufferView, R>)
        and std::is_same_v<typename L::const_iterator::value_type,
                           typename R::const_iterator::value_type>
  auto operator<=>(const L &lhs, const R &rhs) noexcept {
    return cxx20::lexicographical_compare_three_way(
        std::cbegin(lhs), std::cend(lhs), std::cbegin(rhs), std::cend(rhs));
  }

  template <typename L, typename R>
    requires(std::is_base_of_v<BufferView, L>
             or std::is_base_of_v<BufferView, R>)
        and std::is_same_v<typename L::const_iterator::value_type,
                           typename R::const_iterator::value_type>
  auto operator==(const L &lhs, const R &rhs) noexcept {
    return (lhs <=> rhs) == std::strong_ordering::equal;
  }

  inline std::ostream &operator<<(std::ostream &os, BufferView view) {
    return os << view.toHex();
  }

}  // namespace kagome::common

template <>
struct fmt::formatter<kagome::common::BufferView> {
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
  auto format(const kagome::common::BufferView &view, FormatContext &ctx) const
      -> decltype(ctx.out()) {
    // ctx.out() is an output iterator to write to.

    if (view.empty()) {
      static constexpr string_view message("<empty>");
      return std::copy(std::begin(message), std::end(message), ctx.out());
    }

    if (presentation == 's' && view.size() > 5) {
      return fmt::format_to(
          ctx.out(),
          "0x{:04x}…{:04x}",
          // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
          htobe16(*reinterpret_cast<const uint16_t *>(view.data())),
          // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
          htobe16(*reinterpret_cast<const uint16_t *>(view.data() + view.size()
                                                      - sizeof(uint16_t))));
    }

    return fmt::format_to(ctx.out(), "0x{}", view.toHex());
  }
};
