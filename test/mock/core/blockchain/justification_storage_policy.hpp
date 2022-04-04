/**
* Copyright Soramitsu Co., Ltd. All Rights Reserved.
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef KAGOME_MOCK_BLOCKCHAIN_JUSTIFICATION_STORAGE_POLICY_HPP
#define KAGOME_MOCK_BLOCKCHAIN_JUSTIFICATION_STORAGE_POLICY_HPP

#include <gmock/gmock.h>

#include "blockchain/impl/justification_storage_policy.hpp"

namespace kagome::blockchain {

  class JustificationStoragePolicyMock: public JustificationStoragePolicy {
   public:

    MOCK_METHOD(bool, shouldStore, (primitives::BlockInfo block), (const));

   private:

  };

}

#endif  // KAGOME_MOCK_BLOCKCHAIN_JUSTIFICATION_STORAGE_POLICY_HPP
