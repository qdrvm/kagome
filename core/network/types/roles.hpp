/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_NETWORK_TYPES_ROLES_HPP
#define KAGOME_CORE_NETWORK_TYPES_ROLES_HPP

#include "scale/tie.hpp"

namespace kagome::network {

  union Roles {
    SCALE_TIE_ONLY(value);

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
}  // namespace kagome::network

#endif  // KAGOME_CORE_NETWORK_TYPES_ROLES_HPP
