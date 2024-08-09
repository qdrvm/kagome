/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <array>

#include <fmt/format.h>
#include <boost/functional/hash.hpp>
#include <scale/scale.hpp>

#include "common/buffer_view.hpp"
#include "common/hexutil.hpp"
#include "macro/endianness_utils.hpp"

#define KAGOME_BLOB_STRICT_TYPEDEF(space_name, class_name, blob_size)          \
  namespace space_name {                                                       \
    struct class_name : public ::kagome::common::Blob<blob_size> {             \
      using Base = ::kagome::common::Blob<blob_size>;                          \
                                                                               \
      class_name() = default;                                                  \
      class_name(const class_name &) = default;                                \
      class_name(class_name &&) = default;                                     \
      class_name &operator=(const class_name &) = default;                     \
      class_name &operator=(class_name &&) = default;                          \
                                                                               \
      explicit class_name(const Base &blob) : Base{blob} {}                    \
      explicit class_name(Base &&blob) : Base{std::move(blob)} {}              \
                                                                               \
      ~class_name() = default;                                                 \
                                                                               \
      class_name &operator=(const Base &blob) {                                \
        Blob::operator=(blob);                                                 \
        return *this;                                                          \
      }                                                                        \
                                                                               \
      class_name &operator=(Base &&blob) {                                     \
        Blob::operator=(std::move(blob));                                      \
        return *this;                                                          \
      }                                                                        \
                                                                               \
      static ::outcome::result<class_name> fromString(std::string_view data) { \
        OUTCOME_TRY(blob, Base::fromString(data));                             \
        return class_name{std::move(blob)};                                    \
      }                                                                        \
                                                                               \
      static ::outcome::result<class_name> fromHex(std::string_view hex) {     \
        OUTCOME_TRY(blob, Base::fromHex(hex));                                 \
        return class_name{std::move(blob)};                                    \
      }                                                                        \
                                                                               \
      static ::outcome::result<class_name> fromHexWithPrefix(                  \
          std::string_view hex) {                                              \
        OUTCOME_TRY(blob, Base::fromHexWithPrefix(hex));                       \
        return class_name{std::move(blob)};                                    \
      }                                                                        \
                                                                               \
      static ::outcome::result<class_name> fromSpan(                           \
          const common::BufferView &span) {                                    \
        OUTCOME_TRY(blob, Base::fromSpan(span));                               \
        return class_name{std::move(blob)};                                    \
      }                                                                        \
                                                                               \
      friend inline ::scale::ScaleEncoderStream &operator<<(                   \
          ::scale::ScaleEncoderStream &s,                                      \
          const space_name::class_name &data) {                                \
        return s << static_cast<const Base &>(data);                           \
      }                                                                        \
                                                                               \
      friend inline ::scale::ScaleDecoderStream &operator>>(                   \
          ::scale::ScaleDecoderStream &s, space_name::class_name &data) {      \
        return s >> static_cast<Base &>(data);                                 \
      }                                                                        \
    };                                                                         \
  };                                                                           \
                                                                               \
  template <>                                                                  \
  struct std::hash<space_name::class_name> {                                   \
    auto operator()(const space_name::class_name &key) const {                 \
      /* NOLINTNEXTLINE */                                                     \
      return boost::hash_range(key.cbegin(), key.cend());                      \
    }                                                                          \
  };                                                                           \
                                                                               \
  template <>                                                                  \
  struct fmt::formatter<space_name::class_name>                                \
      : fmt::formatter<space_name::class_name::Base> {                         \
    template <typename FormatCtx>                                              \
    auto format(const space_name::class_name &blob, FormatCtx &ctx) const      \
        -> decltype(ctx.out()) {                                               \
      return fmt::formatter<space_name::class_name::Base>::format(blob, ctx);  \
    }                                                                          \
  };

namespace kagome::common {

  /**
   * Error codes for exceptions that may occur during blob initialization
   */
  enum class BlobError { INCORRECT_LENGTH = 1 };

  using byte_t = uint8_t;

  /**
   * Base type which represents blob of fixed size.
   *
   * std::string is convenient to use but it is not safe.
   * We can not specify the fixed length for string.
   *
   * For std::array it is possible, so we prefer it over std::string.
   */
  template <size_t size_>
  class Blob : public std::array<byte_t, size_> {
    using Array = std::array<byte_t, size_>;

   public:
    // Next line is required at least for the scale-codec
    static constexpr bool is_static_collection = true;

    using const_narref = const byte_t (&)[size_];
    using const_narptr = const byte_t (*)[size_];
    /**
     * Initialize blob value
     */
    constexpr Blob() : Array{} {}

    const_narref internal_array_reference() const {
      return *const_narptr(this->data());
    }

    /**
     * @brief constructor enabling initializer list
     * @param l initializer list
     */
    constexpr explicit Blob(const Array &l) : Array{l} {}

    /**
     * In compile-time returns size of current blob.
     */
    static constexpr size_t size() {
      return size_;
    }

    /**
     * Converts current blob to std::string
     */
    std::string toString() const {
      return std::string{this->begin(), this->end()};
    }

    /**
     * Converts current blob to hex string.
     */
    std::string toHex() const {
      return hex_lower({this->begin(), this->end()});
    }

    /**
     * Create Blob from arbitrary string, putting its bytes into the blob
     * @param data arbitrary string containing
     * @return result containing Blob object if string has proper size
     */
    static outcome::result<Blob<size_>> fromString(std::string_view data) {
      if (data.size() != size_) {
        return BlobError::INCORRECT_LENGTH;
      }

      Blob<size_> b;
      std::copy(data.begin(), data.end(), b.begin());

      return b;
    }

    /**
     * Create Blob from hex string
     * @param hex hex string
     * @return result containing Blob object if hex string has proper size and
     * is in hex format
     */
    static outcome::result<Blob<size_>> fromHex(std::string_view hex) {
      OUTCOME_TRY(res, unhex(hex));
      return fromSpan(res);
    }

    /**
     * Create Blob from hex string prefixed with 0x
     * @param hex hex string
     * @return result containing Blob object if hex string has proper size and
     * is in hex format
     */
    static outcome::result<Blob<size_>> fromHexWithPrefix(
        std::string_view hex) {
      OUTCOME_TRY(res, unhexWith0x(hex));
      return fromSpan(res);
    }

    /**
     * Create Blob from BufferView
     */
    static outcome::result<Blob<size_>> fromSpan(const BufferView &span) {
      if (span.size() != size_) {
        return BlobError::INCORRECT_LENGTH;
      }

      Blob<size_> blob;
      std::copy(span.begin(), span.end(), blob.begin());
      return blob;
    }
  };

  // extern specification of the most frequently instantiated blob
  // specializations, used mostly for Hash instantiation
  extern template class Blob<8ul>;
  extern template class Blob<16ul>;
  extern template class Blob<32ul>;
  extern template class Blob<64ul>;

  // Hash specializations
  using Hash64 = Blob<8>;
  using Hash128 = Blob<16>;
  using Hash256 = Blob<32>;
  using Hash512 = Blob<64>;

  template <size_t N>
  inline std::ostream &operator<<(std::ostream &os, const Blob<N> &blob) {
    return os << blob.toHex();
  }

}  // namespace kagome::common

namespace kagome {
  using common::Hash256;
}  // namespace kagome

template <size_t N>
struct std::hash<kagome::common::Blob<N>> {
  auto operator()(const kagome::common::Blob<N> &blob) const {
    return boost::hash_range(blob.data(), blob.data() + N);  // NOLINT
  }
};

template <size_t N>
struct fmt::formatter<kagome::common::Blob<N>> {
  // Presentation format: 's' - short, 'l' - long.
  char presentation = N > 4 ? 's' : 'l';

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
  auto format(const kagome::common::Blob<N> &blob, FormatContext &ctx) const
      -> decltype(ctx.out()) {
    // ctx.out() is an output iterator to write to.

    if (presentation == 's') {
      return fmt::format_to(
          ctx.out(),
          "0x{:04x}…{:04x}",
          // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
          htobe16(*reinterpret_cast<const uint16_t *>(blob.data())),
          // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
          htobe16(*reinterpret_cast<const uint16_t *>(blob.data() + blob.size()
                                                      - sizeof(uint16_t))));
    }

    return fmt::format_to(ctx.out(), "0x{}", blob.toHex());
  }
};

OUTCOME_HPP_DECLARE_ERROR(kagome::common, BlobError);
