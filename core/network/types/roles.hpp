/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

// NOLINTBEGIN(cppcoreguidelines-pro-type-union-access)
namespace kagome::network {

  struct Roles {
    union {
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
    };

    Roles() : value(0) {}
    Roles(uint8_t v) : value(v) {}

    SCALE_CUSTOM_DECOMPOSITION(Roles, value);

    // https://github.com/paritytech/polkadot-sdk/blob/6c3219ebe9231a0305f53c7b33cb558d46058062/substrate/client/network/common/src/role.rs#L101
    bool isFull() const {
      return flags.full != 0 or isAuthority();
    }

    bool isAuthority() const {
      return flags.authority != 0;
    }

    // https://github.com/paritytech/polkadot-sdk/blob/6c3219ebe9231a0305f53c7b33cb558d46058062/substrate/client/network/common/src/role.rs#L111
    bool isLight() const {
      return not isFull();
    }
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
// NOLINTEND(cppcoreguidelines-pro-type-union-access)
