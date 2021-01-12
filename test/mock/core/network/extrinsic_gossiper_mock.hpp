/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_CORE_NETWORK_EXTRINSIC_GOSSIPER_MOCK_HPP
#define KAGOME_TEST_CORE_NETWORK_EXTRINSIC_GOSSIPER_MOCK_HPP

#include <gmock/gmock.h>

#include "network/extrinsic_gossiper.hpp"

namespace kagome::network {

  class ExtrinsicGossiperMock : public ExtrinsicGossiper {
   public:
    ~ExtrinsicGossiperMock() override = default;

    MOCK_METHOD1(propagateTransactions,
                 void(gsl::span<const primitives::Transaction>));
  };

}  // namespace kagome::network

#endif  // KAGOME_TEST_CORE_NETWORK_EXTRINSIC_GOSSIPER_MOCK_HPP
