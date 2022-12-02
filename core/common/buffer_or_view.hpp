/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_COMMON_BUFFER_OR_VIEW_HPP
#define KAGOME_COMMON_BUFFER_OR_VIEW_HPP

#include <variant>

#include "common/buffer.hpp"

namespace kagome::common {
  class BufferOrView {
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

    bool owned() const {
      return variant.index() == 1;
    }

    BufferView view() const {
      if (!owned()) {
        return std::get<BufferView>(variant);
      }
      return BufferView{std::get<Buffer>(variant)};
    }

    operator BufferView() const {
      return view();
    }

    size_t size() const {
      return view().size();
    }

    // get mutable buffer reference, copy once if view
    Buffer &mut() {
      if (!owned()) {
        auto view = std::get<BufferView>(variant);
        variant.emplace<Buffer>(view);
      }
      return std::get<Buffer>(variant);
    }

    // move buffer away, copy once if view
    Buffer into() {
      auto buffer = std::move(mut());
      variant.emplace<BufferView>();
      return buffer;
    }

   private:
    std::variant<BufferView, Buffer> variant;
  };
}  // namespace kagome::common

template <>
struct fmt::formatter<kagome::common::BufferOrView>
    : fmt::formatter<kagome::common::BufferView> {};

#endif  // KAGOME_COMMON_BUFFER_OR_VIEW_HPP
