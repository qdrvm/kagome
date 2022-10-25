/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BUFFERVIEW_HPP
#define KAGOME_BUFFERVIEW_HPP

#include <gsl/span>

#include "common/hexutil.hpp"

namespace kagome::common {
  template <size_t MaxSize>
  class BufferT;
}

namespace kagome::common {

  class BufferView : public gsl::span<const uint8_t>
  {
    using Span = gsl::span<const uint8_t>;
    std::optional<std::vector<Span::value_type>> holder_{};

   public:
    BufferView() = default;

    BufferView(const Span &other) noexcept : Span(other) {}

    BufferView(const std::vector<Span::value_type> &vec) noexcept
        : Span(vec.data(), static_cast<Span::index_type>(vec.size())) {}

    BufferView(std::vector<Span::value_type> &&vec) noexcept
        : holder_(std::move(vec)) {
      static_cast<Span &>(*this) =
          Span(holder_->data(), static_cast<Span::index_type>(holder_->size()));
    }

    template <size_t Size>
    BufferView(const std::array<Span::value_type, Size> &arr) noexcept
        : Span(arr.data(), static_cast<Span::index_type>(arr.size())) {}

    BufferView &operator=(const std::vector<Span::value_type> &vec) noexcept {
      holder_.reset();
      static_cast<Span &>(*this) =
          Span(vec.data(), static_cast<Span::index_type>(vec.size()));
      return *this;
    }

    BufferView &operator=(std::vector<Span::value_type> &&vec) noexcept {
      holder_.emplace(std::move(vec));
      static_cast<Span &>(*this) =
          Span(holder_->data(), static_cast<Span::index_type>(holder_->size()));
      return *this;
    }

    template <size_t Size>
    BufferView &operator=(
        const std::array<Span::value_type, Size> &arr) noexcept {
      holder_.reset();
      static_cast<Span &>(*this) =
          Span(arr.data(), static_cast<Span::index_type>(arr.size()));
      return *this;
    }

    std::string toHex() const {
      return hex_lower(*this);
    }

    template <typename Container1,
              typename Container2,
              typename = std::enable_if<
                  std::is_same_v<typename Container1::value_type, value_type>>,
              typename = std::enable_if<
                  std::is_same_v<typename Container1::value_type,
                                 typename Container2::value_type>>>
    friend bool operator==(const Container1 &lhs,
                           const Container2 &rhs) noexcept {
      return std::equal(lhs.cbegin(), lhs.cend(), rhs.cbegin(), rhs.cend());
    }

    bool operator<(const BufferView &other) const noexcept {
      return std::lexicographical_compare(
          cbegin(), cend(), other.cbegin(), other.cend());
    }

    template <typename Container,
              typename = std::enable_if<
                  std::is_same_v<typename Container::value_type, value_type>>>
    bool operator<(const Container &other) const noexcept {
      return std::lexicographical_compare(
          cbegin(), cend(), other.cbegin(), other.cend());
    }

    template <typename Container,
              typename = std::enable_if<
                  std::is_same_v<typename Container::value_type, value_type>>>
    friend bool operator<(const Container &lhs,
                          const BufferView &rhs) noexcept {
      return std::lexicographical_compare(
          lhs.cbegin(), lhs.cend(), rhs.cbegin(), rhs.cend());
    }

  };

  inline std::ostream &operator<<(std::ostream &os, BufferView view){
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
  auto format(const kagome::common::BufferView &view, FormatContext &ctx)
      -> decltype(ctx.out()) {
    // ctx.out() is an output iterator to write to.

    if (view.empty()) {
      return format_to(ctx.out(), "empty");
    }

    if (presentation == 's' && view.size() > 5) {
      return format_to(
          ctx.out(),
          "0x{:04x}…{:04x}",
          // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
          htobe16(*reinterpret_cast<const uint16_t *>(view.data())),
          // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
          htobe16(*reinterpret_cast<const uint16_t *>(view.data() + view.size()
                                                      - sizeof(uint16_t))));
    }

    return format_to(ctx.out(), "0x{}", view.toHex());
  }
};

#endif  // KAGOME_BUFFERVIEW_HPP
