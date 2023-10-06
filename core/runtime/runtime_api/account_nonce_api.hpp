/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "primitives/account.hpp"

#include "primitives/block_id.hpp"

namespace kagome::runtime {

  class AccountNonceApi {
   public:
    virtual ~AccountNonceApi() = default;

    virtual outcome::result<primitives::AccountNonce> account_nonce(
        const primitives::BlockHash &block,
        const primitives::AccountId &account_id) = 0;
  };

}  // namespace kagome::runtime
