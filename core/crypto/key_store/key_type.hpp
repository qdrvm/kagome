/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/buffer.hpp"
#include "outcome/outcome.hpp"
#include "scale/scale.hpp"

namespace kagome::crypto {

  enum class KeyTypeError : uint8_t {
    UNSUPPORTED_KEY_TYPE = 1,
  };

  struct KeyType {
    KeyType() = default;
    constexpr KeyType(uint32_t id) : id_(id){};

    static constexpr std::optional<KeyType> fromString(std::string_view s) {
      if (s.size() != 4) {
        return std::nullopt;
      }
      return KeyType((static_cast<uint32_t>(s[0]) << (CHAR_BIT * 0))
                     | (static_cast<uint32_t>(s[1]) << (CHAR_BIT * 1))
                     | (static_cast<uint32_t>(s[2]) << (CHAR_BIT * 2))
                     | (static_cast<uint32_t>(s[3]) << (CHAR_BIT * 3)));
    }

    constexpr operator uint32_t() const {
      return id_;
    }

    bool is_supported() const;

    constexpr auto operator<=>(const KeyType &other) const = default;

    friend inline ::scale::ScaleEncoderStream &operator<<(
        ::scale::ScaleEncoderStream &s, const KeyType &v) {
      return s << v.id_;
    }

    friend inline ::scale::ScaleDecoderStream &operator>>(
        ::scale::ScaleDecoderStream &s, KeyType &v) {
      return s >> v.id_;
    }

    std::string toString() const {
      return {reinterpret_cast<const char *>(&id_), 4};
    }

   private:
    uint32_t id_{0};
  };

  /**
   * Makes 32bit integer which represent encoded 4-char strings
   * Little-endian byte order is used
   */
  consteval uint32_t operator""_key(const char *s, size_t len) {
    return *KeyType::fromString(s);
  }

  struct KeyTypes {
    /// Key type for Babe module, built-in.
    static constexpr KeyType BABE = "babe"_key;
    /// Key type for Sassafras module, built-in.
    static constexpr KeyType SASSAFRAS = "sass"_key;
    /// Key type for Grandpa module, built-in.
    static constexpr KeyType GRANDPA = "gran"_key;
    /// Key type for controlling an account in a Substrate runtime, built-in.
    static constexpr KeyType ACCOUNT = "acco"_key;
    /// Key type for Aura module, built-in.
    static constexpr KeyType AURA = "aura"_key;
    /// Key type for BEEFY module.
    static constexpr KeyType BEEFY = "beef"_key;
    /// Key type for ImOnline module, built-in.
    static constexpr KeyType IM_ONLINE = "imon"_key;
    /// Key type for AuthorityDiscovery module, built-in.
    static constexpr KeyType AUTHORITY_DISCOVERY = "audi"_key;
    /// Key type for staking, built-in.
    static constexpr KeyType STAKING = "stak"_key;
    /// A key type for signing statements
    static constexpr KeyType STATEMENT = "stmt"_key;
    /// The key type ID for parachain assignment key.
    static constexpr KeyType ASSIGNMENT = "asgn"_key;
    /// The key type ID for a parachain validator key.
    static constexpr KeyType PARACHAIN = "para"_key;
    /// A key type ID useful for tests.
    static constexpr KeyType DUMMY = "dumy"_key;

    KeyTypes() = delete;

    static constexpr bool is_supported(const KeyType &key_type) {
      switch (key_type) {
        case KeyTypes::BABE:
        case KeyTypes::SASSAFRAS:
        case KeyTypes::GRANDPA:
        case KeyTypes::ACCOUNT:
        case KeyTypes::AURA:
        case KeyTypes::BEEFY:
        case KeyTypes::IM_ONLINE:
        case KeyTypes::AUTHORITY_DISCOVERY:
        case KeyTypes::STAKING:
        case KeyTypes::STATEMENT:
        case KeyTypes::ASSIGNMENT:
        case KeyTypes::PARACHAIN:
        case KeyTypes::DUMMY:
          return true;
      }
      return false;
    }
  };

  std::string encodeKeyFileName(const KeyType &type, common::BufferView key);

  outcome::result<std::pair<KeyType, common::Buffer>> decodeKeyFileName(
      std::string_view name);
}  // namespace kagome::crypto

template <>
struct std::hash<kagome::crypto::KeyType> {
  std::size_t operator()(const kagome::crypto::KeyType &key_type) const {
    return std::hash<uint32_t>()(key_type);
  }
};

template <>
struct fmt::formatter<kagome::crypto::KeyType> {
  // Parses format specifications. Must be empty
  constexpr auto parse(format_parse_context &ctx) -> decltype(ctx.begin()) {
    // Parse the presentation format and store it in the formatter:
    auto it = ctx.begin(), end = ctx.end();

    // Check if reached the end of the range:
    if (it != end && *it != '}') {
      throw format_error("invalid format");
    }

    // Return an iterator past the end of the parsed range:
    return it;
  }

  // Formats the optional value
  template <typename FormatContext>
  auto format(const kagome::crypto::KeyType &key_type, FormatContext &ctx) const
      -> decltype(ctx.out()) {
    auto u32_rep = (uint32_t)key_type;
    auto is_printable =
        std::isprint(static_cast<char>(u32_rep >> (CHAR_BIT * 0)))
        && std::isprint(static_cast<char>(u32_rep >> (CHAR_BIT * 1)))
        && std::isprint(static_cast<char>(u32_rep >> (CHAR_BIT * 2)))
        && std::isprint(static_cast<char>(u32_rep >> (CHAR_BIT * 3)));

    if (is_printable) {
      std::string_view ascii(
          reinterpret_cast<const char *>(&u32_rep),  // NOLINT
          sizeof(u32_rep));
      return fmt::format_to(
          ctx.out(), "<hex: {:08x}, ascii: '{}'>", u32_rep, ascii);
    }

    return fmt::format_to(ctx.out(), "<hex: {:08x}>", u32_rep);
  }
};

OUTCOME_HPP_DECLARE_ERROR(kagome::crypto, KeyTypeError);
