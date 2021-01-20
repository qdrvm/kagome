/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_RUNTIME_SYSTEM_HPP
#define KAGOME_RUNTIME_SYSTEM_HPP

#include "primitives/account.hpp"

namespace kagome::runtime {

  class System {
   public:
    virtual ~System() = default;

    virtual outcome::result<primitives::AccountNonce> account_nonce(
        const primitives::AccountId & account_id) = 0;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_SYSTEM_HPP
