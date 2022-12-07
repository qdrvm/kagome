/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_NETWORK_TYPES_ROLES_HPP
#define KAGOME_CORE_NETWORK_TYPES_ROLES_HPP

namespace kagome::network {

  union Roles {
    struct {
      /**
       * Full node, does not participate in consensus.
       */
      uint8_t full : 1;

      /**
       * Light client node.
       */
      uint8_t light : 1;

      /**
       * Act as an authority
       */
      uint8_t authority : 1;

    } flags;
    uint8_t value;

    Roles() : value(0) {}
    Roles(uint8_t v) : value(v) {}
  };

  inline std::string to_string(Roles r) {
    switch (r.value) {
      case 0:
        return "none";
      case 1:
        return "full";
      case 2:
        return "light";
      case 4:
        return "authority";
    }
    return to_string(r.value);
  }

  /**
   * @brief compares two Roles instances
   * @param lhs first instance
   * @param rhs second instance
   * @return true if equal false otherwise
   */
  inline bool operator==(const Roles &lhs, const Roles &rhs) {
    return lhs.value == rhs.value;
  }

  /**
   * @brief outputs object of type Roles to stream
   * @tparam Stream output stream type
   * @param s stream reference
   * @param v value to output
   * @return reference to stream
   */
  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const Roles &v) {
    return s << v.value;
  }

  /**
   * @brief decodes object of type Roles from stream
   * @tparam Stream input stream type
   * @param s stream reference
   * @param v value to decode
   * @return reference to stream
   */
  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, Roles &v) {
    return s >> v.value;
  }

}  // namespace kagome::network

#endif  // KAGOME_CORE_NETWORK_TYPES_ROLES_HPP
