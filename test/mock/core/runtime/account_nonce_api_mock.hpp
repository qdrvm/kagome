/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_RUNTIME_SYSTEM_MOCK
#define KAGOME_RUNTIME_SYSTEM_MOCK

#include "runtime/runtime_api/account_nonce_api.hpp"

#include <gmock/gmock.h>

namespace kagome::runtime {

  class AccountNonceApiMock : public AccountNonceApi {
   public:
    MOCK_METHOD(outcome::result<primitives::AccountNonce>,
                account_nonce,
                (const primitives::BlockHash &block,
                 const primitives::AccountId &),
                (override));
  };

}  // namespace kagome::runtime

#endif  // KAGOME_RUNTIME_SYSTEM_MOCK
