/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <bitset>
#include <cstdint>

#include "common/outcome_throw.hpp"
#include "scale/scale_error.hpp"

namespace kagome::network {

  /**
   * Masks of bits, combination of which shows, which fields are to be presented
   * in the BlockResponse
   */
  enum class BlockAttribute : uint8_t {
    /// Include block header.
    HEADER = 1u,
    /// Include block body.
    BODY = 1u << 1u,
    /// Include block receipt.
    RECEIPT = 1u << 2u,
    /// Include block message queue.
    MESSAGE_QUEUE = 1u << 3u,
    /// Include a justification for the block.
    JUSTIFICATION = 1u << 4u
  };

  /**
   * Block attributes
   */
  class BlockAttributes {
   public:
    BlockAttributes() = default;
    constexpr BlockAttributes(const BlockAttributes &other) noexcept = default;
    BlockAttributes(BlockAttributes &&other) noexcept = default;
    ~BlockAttributes() = default;

    constexpr BlockAttributes(BlockAttribute attribute) noexcept
        : attributes(static_cast<uint8_t>(attribute)) {}

    template <typename T, typename = std::enable_if<std::is_unsigned_v<T>>>
    constexpr BlockAttributes(T attribute) noexcept : attributes(0) {
      load(attribute);
    }

    inline constexpr BlockAttributes &operator=(const BlockAttributes &other) =
        default;
    inline constexpr BlockAttributes &operator=(BlockAttributes &&other) =
        default;

    template <typename T, typename = std::enable_if<std::is_unsigned_v<T>>>
    constexpr void load(T value) {
      attributes = mask_ & value;
    }

    inline BlockAttributes &operator=(BlockAttribute attribute) {
      attributes = static_cast<uint8_t>(attribute);
      return *this;
    }

    inline constexpr BlockAttributes operator|(
        const BlockAttributes &other) const {
      BlockAttributes result;
      result.attributes = attributes | static_cast<uint8_t>(other.attributes);
      return result;
    }

    inline constexpr BlockAttributes operator|(
        const BlockAttribute &attribute) const {
      BlockAttributes result;
      result.attributes = attributes | static_cast<uint8_t>(attribute);
      return result;
    }

    inline BlockAttributes &operator|=(const BlockAttributes &other) {
      attributes |= other.attributes;
      return *this;
    }

    inline BlockAttributes &operator|=(const BlockAttribute &attribute) {
      attributes |= static_cast<uint8_t>(attribute);
      return *this;
    }

    inline constexpr BlockAttributes operator&(
        const BlockAttributes &other) const {
      BlockAttributes result;
      result.attributes = attributes & static_cast<uint8_t>(other.attributes);
      return result;
    }

    inline constexpr BlockAttributes operator&(
        const BlockAttribute &attribute) const {
      BlockAttributes result;
      result.attributes = attributes & static_cast<uint8_t>(attribute);
      return result;
    }

    inline BlockAttributes &operator&=(const BlockAttributes &other) {
      attributes &= other.attributes;
      return *this;
    }

    inline BlockAttributes &operator&=(BlockAttribute attribute) {
      attributes &= static_cast<uint8_t>(attribute);
      return *this;
    }

    inline BlockAttributes operator~() const {
      BlockAttributes result;
      result.attributes = ~attributes & mask_;
      return result;
    }

    inline bool operator==(const BlockAttributes &other) const {
      return attributes == other.attributes;
    }

    inline operator bool() const {
      return attributes != 0;
    }

    template <typename T, typename = std::enable_if<std::is_unsigned_v<T>>>
    inline operator T() const {
      return attributes;
    }

   private:
    static constexpr uint8_t mask_ = 0b00011111;
    uint8_t attributes = 0;

    friend struct std::hash<BlockAttributes>;
  };

  inline constexpr BlockAttributes operator|(const BlockAttribute &lhs,
                                             const BlockAttribute &rhs) {
    BlockAttributes result;
    result.load(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
    return result;
  }

  inline constexpr BlockAttributes operator~(const BlockAttribute &attribute) {
    BlockAttributes result;
    result.load(~static_cast<uint8_t>(attribute));
    return result;
  }

  /**
   * @brief outputs object of type BlockAttributes to stream
   * @tparam Stream output stream type
   * @param s stream reference
   * @param v value to output
   * @return reference to stream
   */
  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const BlockAttributes &v) {
    return s << static_cast<uint8_t>(v);
  }

  /**
   * decodes object of type BlockAttributes from stream
   * @tparam Stream input stream type
   * @param s stream reference
   * @param v value to decode
   * @return reference to stream
   */
  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, BlockAttributes &attributes) {
    uint8_t value = 0u;
    s >> value;

    attributes.load(value);
    if ((uint8_t)attributes != value) {
      common::raise(scale::DecodeError::UNEXPECTED_VALUE);
    }
    return s;
  }

}  // namespace kagome::network

template <>
struct std::hash<kagome::network::BlockAttributes> {
  auto operator()(const kagome::network::BlockAttributes &attr) const {
    return std::hash<uint8_t>()(attr.attributes);
  }
};
