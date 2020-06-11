/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_MOCK_CORE_CONSENSUS_BABE_BABE_SYNCHRONIZER_MOCK_HPP
#define KAGOME_TEST_MOCK_CORE_CONSENSUS_BABE_BABE_SYNCHRONIZER_MOCK_HPP

#include "consensus/babe/babe_synchronizer.hpp"

#include <gmock/gmock.h>

namespace kagome::consensus {

  class BabeSynchronizerMock : public BabeSynchronizer {
   public:
    MOCK_METHOD4(request,
                 void(const primitives::BlockId &,
                      const primitives::BlockHash &,
                      primitives::AuthorityIndex,
                      const BlocksHandler &));
  };

}  // namespace kagome::consensus

#endif  // KAGOME_TEST_MOCK_CORE_CONSENSUS_BABE_BABE_SYNCHRONIZER_MOCK_HPP
