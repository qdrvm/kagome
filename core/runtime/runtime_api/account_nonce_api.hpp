/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_RUNTIME_ACCOUNTNONCEAPI_HPP
#define KAGOME_RUNTIME_ACCOUNTNONCEAPI_HPP

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

#endif  // KAGOME_ACCOUNTNONCEAPI_HPP
