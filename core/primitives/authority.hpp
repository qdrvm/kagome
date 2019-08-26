/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_AUTHORITY_HPP
#define KAGOME_AUTHORITY_HPP

#include "../../../../../../Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/include/c++/v1/cstdint"

#include "common.hpp"

namespace kagome::primitives {
  using AuthorityWeight = uint64_t;

  /**
   * Authority, which participate in block production and finalization
   */
  struct Authority {
    primitives::AuthorityId id;
    AuthorityWeight babe_weight{};

    bool operator==(const Authority &other) const {
      return id == other.id && babe_weight == other.babe_weight;
    }
    bool operator!=(const Authority &other) const {
      return !(*this == other);
    }
  };
}  // namespace kagome::primitives

#endif  // KAGOME_AUTHORITY_HPP
