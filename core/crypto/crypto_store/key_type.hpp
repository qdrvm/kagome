/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CRYPTO_KEY_TYPE_HPP
#define KAGOME_CRYPTO_KEY_TYPE_HPP

#include "common/buffer.hpp"
#include "outcome/outcome.hpp"

namespace kagome::crypto {

  enum class KeyTypeError {
    UNSUPPORTED_KEY_TYPE = 1,
    UNSUPPORTED_KEY_TYPE_ID,
  };

  /// Key type identifier
  using KeyTypeId = uint32_t;

  /**
   * Makes 32bit integer KeyTypeId which represent encoded 4-char strings
   * Little-endian byte order is used
   */
  constexpr KeyTypeId operator""_key(const char *s, std::size_t size) {
    return (static_cast<KeyTypeId>(s[0]) << (CHAR_BIT * 0))
         | (static_cast<KeyTypeId>(s[1]) << (CHAR_BIT * 1))
         | (static_cast<KeyTypeId>(s[2]) << (CHAR_BIT * 2))
         | (static_cast<KeyTypeId>(s[3]) << (CHAR_BIT * 3));
  }

  class KeyType {
   public:
    /// Key type for Babe module, built-in.
    static constexpr KeyTypeId BABE = "babe"_key;
    /// Key type for Sassafras module, built-in.
    static constexpr KeyTypeId SASSAFRAS = "sass"_key;
    /// Key type for Grandpa module, built-in.
    static constexpr KeyTypeId GRANDPA = "gran"_key;
    /// Key type for controlling an account in a Substrate runtime, built-in.
    static constexpr KeyTypeId ACCOUNT = "acco"_key;
    /// Key type for Aura module, built-in.
    static constexpr KeyTypeId AURA = "aura"_key;
    /// Key type for BEEFY module.
    static constexpr KeyTypeId BEEFY = "beef"_key;
    /// Key type for ImOnline module, built-in.
    static constexpr KeyTypeId IM_ONLINE = "imon"_key;
    /// Key type for AuthorityDiscovery module, built-in.
    static constexpr KeyTypeId AUTHORITY_DISCOVERY = "audi"_key;
    /// Key type for staking, built-in.
    static constexpr KeyTypeId STAKING = "stak"_key;
    /// A key type for signing statements
    static constexpr KeyTypeId STATEMENT = "stmt"_key;
    /// A key type ID useful for tests.
    static constexpr KeyTypeId DUMMY = "dumy"_key;

    static constexpr KeyTypeId KEY_TYPE_ASGN = "asgn"_key;
    static constexpr KeyTypeId KEY_TYPE_PARA = "para"_key;

    constexpr KeyType(KeyTypeId id) : id_(id){};

    constexpr operator KeyTypeId() const {
      return id_;
    }

    static constexpr bool is_supported(KeyTypeId id) {
      switch (id) {
        case BABE:
        case SASSAFRAS:
        case GRANDPA:
        case ACCOUNT:
        case AURA:
        case BEEFY:
        case IM_ONLINE:
        case AUTHORITY_DISCOVERY:
        case STAKING:
        case STATEMENT:
        case DUMMY:
        case KEY_TYPE_ASGN:
        case KEY_TYPE_PARA:
          return true;
      }
      return false;
    }

    inline constexpr bool is_supported() const {
      return is_supported(id_);
    }

   private:
    KeyTypeId id_;
  };

  /**
   * @brief makes string representation of KeyTypeId
   * @param key_type_id KeyTypeId
   * @return string representation of KeyTypeId
   */
  std::string encodeKeyTypeIdToStr(KeyType key_type);

  /**
   * @brief restores KeyTypeId from its string representation
   * @param param string representation of key type
   * @return KeyTypeId
   */
  KeyType decodeKeyTypeIdFromStr(std::string_view str);

  std::string encodeKeyFileName(KeyType type, common::BufferView key);

  outcome::result<std::pair<KeyType, common::Buffer>> decodeKeyFileName(
      std::string_view name);
}  // namespace kagome::crypto

OUTCOME_HPP_DECLARE_ERROR(kagome::crypto, KeyTypeError);

#endif  // KAGOME_CRYPTO_KEY_TYPE_HPP
