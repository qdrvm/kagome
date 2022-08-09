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

    MOCK_METHOD(std::shared_ptr<BlockAnnounceProtocol>,
                getBlockAnnounceProtocol,
                (),
                (const, override));


    MOCK_METHOD(std::shared_ptr<CollationProtocol>,
                getCollationProtocol,
                (),
                (const, override));

    MOCK_METHOD(std::shared_ptr<PropagateTransactionsProtocol>,
                getPropagateTransactionsProtocol,
                (),
                (const, override));

    MOCK_METHOD(std::shared_ptr<StateProtocol>,
                getStateProtocol,
                (),
                (const, override));

    MOCK_METHOD(std::shared_ptr<SyncProtocol>,
                getSyncProtocol,
                (),
                (const, override));

    MOCK_METHOD(std::shared_ptr<GrandpaProtocol>,
                getGrandpaProtocol,
                (),
                (const, override));

    MOCK_METHOD(std::shared_ptr<libp2p::protocol::Ping>,
                getPingProtocol,
                (),
                (const, override));
  };
}  // namespace kagome::network

#endif  // KAGOME_NETWORK_ROUTERMOCK
