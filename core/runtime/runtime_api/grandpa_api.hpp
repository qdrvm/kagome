/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "primitives/authority.hpp"
#include "primitives/common.hpp"

namespace kagome::runtime {
  // https://github.com/paritytech/substrate/blob/8bf08ca63491961fafe6adf414a7411cb3953dcf/core/finality-grandpa/primitives/src/lib.rs#L56

  /**
   * @brief interface for Grandpa runtime functions
   */
  class GrandpaApi {
   protected:
    using AuthorityList = primitives::AuthorityList;

   public:
    virtual ~GrandpaApi() = default;

    /**
     * @brief calls Grandpa_authorities runtime api function
     * @return collection of current grandpa authorities with their weights
     */
    virtual outcome::result<AuthorityList> authorities(
        const primitives::BlockHash &block_hash) = 0;

    /**
     * @return the id of the current voter set at the provided block
     */
    virtual outcome::result<primitives::AuthoritySetId> current_set_id(
        const primitives::BlockHash &block) = 0;
  };

}  // namespace kagome::runtime
