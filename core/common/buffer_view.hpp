/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <qtils/cxx20/lexicographical_compare_three_way.hpp>
#include <span>

#include "common/hexutil.hpp"
#include "macro/endianness_utils.hpp"

#include <ranges>
#include <span>

inline auto operator""_bytes(const char *s, std::size_t size) {
  return std::span<const uint8_t>(reinterpret_cast<const uint8_t *>(s), size);
}

namespace kagome::common {
  template <size_t MaxSize, typename Allocator>
  class SLBuffer;
}

namespace kagome::common {

  class BufferView : public std::span<const uint8_t> {
   public:
    using span::span;

    BufferView(std::initializer_list<uint8_t> &&) = delete;

    BufferView(const span &other) : span(other) {}

    template <typename T>
      requires std::is_integral_v<std::decay_t<T>> and (sizeof(T) == 1)
    BufferView(std::span<T> other)
        : span(reinterpret_cast<const uint8_t *>(other.data()), other.size()) {}

    template <typename T>
    decltype(auto) operator=(T &&t) {
      return span::operator=(std::forward<T>(t));
    }

    template <size_t count>
    void dropFirst() {
      *this = subspan<count>();
    }

    void dropFirst(size_t count) {
      *this = subspan(count);
    }

    template <size_t count>
    void dropLast() {
      *this = first(size() - count);
    }

    void dropLast(size_t count) {
      *this = first(size() - count);
    }

    std::string toHex() const {
      return hex_lower(*this);
    }

    std::string_view toStringView() const {
      // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
      return {reinterpret_cast<const char *>(data()), size()};
    }

    auto operator<=>(const BufferView &other) const {
      return qtils::cxx20::lexicographical_compare_three_way(
          span::begin(), span::end(), other.begin(), other.end());
    }

    auto operator==(const BufferView &other) const {
      return (*this <=> other) == std::strong_ordering::equal;
    }
  };

  inline std::ostream &operator<<(std::ostream &os, BufferView view) {
    return os << view.toHex();
  }

  template <typename Super, typename Prefix>
  bool startsWith(const Super &super, const Prefix &prefix) {
    if (std::size(super) >= std::size(prefix)) {
      return std::equal(
          std::begin(prefix), std::end(prefix), std::begin(super));
    }
    return false;
  }
}  // namespace kagome::common

namespace kagome {
  using common::BufferView;
}  // namespace kagome

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
