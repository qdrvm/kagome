/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_COMMON_BUFFER_OR_VIEW_HPP
#define KAGOME_COMMON_BUFFER_OR_VIEW_HPP

#include <boost/variant/get.hpp>
#include <boost/variant/variant.hpp>
#include <type_traits>

#include "common/buffer.hpp"

namespace kagome::common {
  /// Moved owned buffer or readonly view.
  class BufferOrView {
    using Span = gsl::span<const uint8_t>;
    template <typename T>
    using AsSpan = std::enable_if_t<std::is_convertible_v<T, Span>>;

    struct Moved {};

   public:
    BufferOrView() = default;

    BufferOrView(const BufferView &view) : variant{view} {}

    BufferOrView(const std::vector<uint8_t> &vector) = delete;
    BufferOrView(std::vector<uint8_t> &&vector)
        : variant{Buffer{std::move(vector)}} {}

    BufferOrView(const BufferOrView &) = delete;
    BufferOrView(BufferOrView &&) = default;

    BufferOrView &operator=(const BufferOrView &) = delete;
    BufferOrView &operator=(BufferOrView &&) = default;

    /// Is buffer owned.
    bool owned() const {
      if (variant.which() == 2) {
        throw std::logic_error{"Tried to use moved BufferOrView"};
      }
      return variant.which() == 1;
    }

    /// Get view.
    BufferView view() const {
      if (!owned()) {
        return boost::get<BufferView>(variant);
      }
      return BufferView{boost::get<Buffer>(variant)};
    }

    /// Get view.
    operator BufferView() const {
      return view();
    }

    /// Byte size.
    size_t size() const {
      return view().size();
    }

    /// Get mutable buffer reference. Copy once if view.
    Buffer &mut() {
      if (!owned()) {
        auto view = boost::get<BufferView>(variant);
        variant = Buffer{view};
      }
      return boost::get<Buffer>(variant);
    }

    /// Move buffer away. Copy once if view.
    Buffer into() {
      auto buffer = std::move(mut());
      variant = Moved{};
      return buffer;
    }

   private:
    boost::variant<BufferView, Buffer, Moved> variant;

    template <typename T, typename = AsSpan<T>>
    friend bool operator==(const BufferOrView &l, const T &r) {
      return l.view() == Span{r};
    }
    template <typename T, typename = AsSpan<T>>
    friend bool operator!=(const BufferOrView &l, const T &r) {
      return l.view() != Span{r};
    }
    template <typename T, typename = AsSpan<T>>
    friend bool operator==(const T &l, const BufferOrView &r) {
      return Span{l} == r.view();
    }
    template <typename T, typename = AsSpan<T>>
    friend bool operator!=(const T &l, const BufferOrView &r) {
      return Span{l} != r.view();
    }
  };
}  // namespace kagome::common

template <>
struct fmt::formatter<kagome::common::BufferOrView>
    : fmt::formatter<kagome::common::BufferView> {};

#endif  // KAGOME_COMMON_BUFFER_OR_VIEW_HPP
