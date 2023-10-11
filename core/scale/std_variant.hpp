/**
 * Copyright Quadrivium LLC All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <variant>

#include <scale.hpp>

namespace scale {

  template <typename Stream, typename... Options>
    requires Stream::is_encoder_stream
  Stream &operator<<(Stream &stream, const std::variant<Options...> &variant) {
    stream << static_cast<uint8_t>(variant.index());
    std::visit([&stream](const auto &v) { stream << v; }, variant);
    return stream;
  }

  template <typename Option, typename Stream, typename... Options>
    requires Stream::is_decoder_stream
  constexpr auto make_decoder() {
    return [](Stream &stream, std::variant<Options...> &variant) {
      variant.template emplace<Option>();
      stream >> std::get<Option>(variant);
    };
  }

  template <typename Stream, typename... Options>
    requires Stream::is_decoder_stream
  Stream &operator>>(Stream &stream, std::variant<Options...> &variant) {
    uint8_t index;
    stream >> index;
    using Decoder =
        void (*)(Stream & stream, std::variant<Options...> & variant);
    constexpr Decoder decoders[]{
        make_decoder<Options, Stream, Options...>()...};
    decoders[index](stream, variant);
    return stream;
  }
}  // namespace scale