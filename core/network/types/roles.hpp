/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>
#include <string>

#include "scale/kagome_scale.hpp"

namespace kagome::network {

  struct Roles {
    /// Full node, does not participate in consensus.
    static constexpr uint8_t Full = 0b0000'0001;
    /// Light client node.
    static constexpr uint8_t Light = 0b0000'0010;
    /// Act as an authority
    static constexpr uint8_t Authority = 0b0000'0100;

    Roles() = default;
    Roles(uint8_t value) : value_(value) {}

    // https://github.com/paritytech/polkadot-sdk/blob/6c3219ebe9231a0305f53c7b33cb558d46058062/substrate/client/network/common/src/role.rs#L101
    bool isFull() const {
      return (value_ & Full) or (value_ & Authority);
    }

    bool isAuthority() const {
      return value_ & Authority;
    }

    // https://github.com/paritytech/polkadot-sdk/blob/6c3219ebe9231a0305f53c7b33cb558d46058062/substrate/client/network/common/src/role.rs#L111
    bool isLight() const {
      return not(value_ & Full);
    }

    uint8_t value() const {
      return value_;
    }

    SCALE_CUSTOM_DECOMPOSITION(Roles, value_);

    friend std::string to_string(Roles roles) {
      switch (roles.value_) {
        case 0:
          return "none";
        case 1:
          return "full";
        case 2:
          return "light";
        case 4:
          return "authority";
        default:
          return std::to_string(roles.value_);
      }
    }

   private:
    uint8_t value_{0};
  };

}  // namespace kagome::network
