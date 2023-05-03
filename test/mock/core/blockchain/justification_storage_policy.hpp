/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_MOCK_BLOCKCHAIN_JUSTIFICATION_STORAGE_POLICY_HPP
#define KAGOME_MOCK_BLOCKCHAIN_JUSTIFICATION_STORAGE_POLICY_HPP

#include <gmock/gmock.h>

#include "blockchain/impl/justification_storage_policy.hpp"

namespace kagome::blockchain {

  class JustificationStoragePolicyMock : public JustificationStoragePolicy {
   public:
    MOCK_METHOD(outcome::result<bool>,
                shouldStoreFor,
                (const primitives::BlockHeader &header,
                 primitives::BlockNumber last_finalized_number),
                (const));

   private:
  };

}  // namespace kagome::blockchain

#endif  // KAGOME_MOCK_BLOCKCHAIN_JUSTIFICATION_STORAGE_POLICY_HPP
