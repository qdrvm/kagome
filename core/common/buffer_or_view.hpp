/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/variant/get.hpp>
#include <boost/variant/variant.hpp>
#include <type_traits>

#include "common/buffer.hpp"

namespace kagome::common {
  /// Moved owned buffer or readonly view.
  class BufferOrView {
    using Span = std::span<const uint8_t>;

    template <typename T>
    using AsSpan = std::enable_if_t<std::is_convertible_v<T, Span>>;

    struct Moved {};

   public:
    BufferOrView() = default;

    BufferOrView(const BufferView &view) : variant{view} {}

    template <size_t N>
    BufferOrView(const std::array<uint8_t, N> &array) : variant{array} {}

    BufferOrView(const std::vector<uint8_t> &vector) = delete;
    BufferOrView(std::vector<uint8_t> &&vector)
        : variant{Buffer{std::move(vector)}} {}

    BufferOrView(const BufferOrView &) = delete;
    BufferOrView(BufferOrView &&) = default;

    BufferOrView &operator=(const BufferOrView &) = delete;
    BufferOrView &operator=(BufferOrView &&) = default;

    /// Is buffer owned.
    bool isOwned() const {
      if (variant.which() == 2) {
        throw std::logic_error{"Tried to use moved BufferOrView"};
      }
      return variant.which() == 1;
    }

    /// Get view.
    BufferView view() const {
      if (!isOwned()) {
        return boost::get<BufferView>(variant);
      }
      return BufferView{boost::get<Buffer>(variant)};
    }

    /// Get view.
    operator BufferView() const {
      return view();
    }

    /// Data ptr for contiguous_range
    auto data() const {
      return view().data();
    }

    /// Size for sized_range
    size_t size() const {
      return view().size();
    }

    /// Iteratior begin for range
    auto begin() const {
      return view().begin();
    }

    /// Iteratior end for range
    auto end() const {
      return view().end();
    }

    /// Get mutable buffer reference. Copy once if view.
    Buffer &mut() {
      if (!isOwned()) {
        auto view = boost::get<BufferView>(variant);
        variant = Buffer{view};
      }
      return boost::get<Buffer>(variant);
    }

    /// Move buffer away. Copy once if view.
    Buffer intoBuffer() {
      auto buffer = std::move(mut());
      variant = Moved{};
      return buffer;
    }

   private:
    boost::variant<BufferView, Buffer, Moved> variant;

    template <typename T, typename = AsSpan<T>>
    friend bool operator==(const BufferOrView &l, const T &r) {
      return std::equal(l.view().begin(), l.view().end(), Span{r}.begin());
    }
    template <typename T, typename = AsSpan<T>>
    friend bool operator==(const T &l, const BufferOrView &r) {
      return std::equal(r.view().begin(), r.view().end(), Span{l}.begin());
    }
  };
}  // namespace kagome::common

template <>
struct fmt::formatter<kagome::common::BufferOrView>
    : fmt::formatter<kagome::common::BufferView> {};
