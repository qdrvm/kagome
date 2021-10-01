/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_ROUTERMOCK
#define KAGOME_NETWORK_ROUTERMOCK

#include "network/router.hpp"

#include <gmock/gmock.h>

namespace kagome::network {
  /**
   * Router, which reads and delivers different network messages to the
   * observers, responsible for their processing
   */
  class RouterMock : public Router {
   public:
    virtual ~RouterMock() = default;

    MOCK_CONST_METHOD0(getBlockAnnounceProtocol,
                       std::shared_ptr<BlockAnnounceProtocol>());
    MOCK_CONST_METHOD0(getPropagateTransactionsProtocol,
                       std::shared_ptr<PropagateTransactionsProtocol>());
    MOCK_CONST_METHOD0(getSyncProtocol, std::shared_ptr<SyncProtocol>());
    MOCK_CONST_METHOD0(getGrandpaProtocol, std::shared_ptr<GrandpaProtocol>());
  };
}  // namespace kagome::network

#endif  // KAGOME_NETWORK_ROUTERMOCK
