/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "network/router.hpp"

#include <gmock/gmock.h>

namespace kagome::network {
  /**
   * Router, which reads and delivers different network messages to the
   * observers, responsible for their processing
   */
  class RouterMock : public Router {
   public:
    MOCK_METHOD(std::shared_ptr<StatementFetchingProtocol>,
                getFetchStatementProtocol,
                (),
                (const, override));

    MOCK_METHOD(std::shared_ptr<FetchAvailableDataProtocol>,
                getFetchAvailableDataProtocol,
                (),
                (const, override));

    MOCK_METHOD(std::shared_ptr<ValidationProtocol>,
                getValidationProtocol,
                (),
                (const, override));

    MOCK_METHOD(std::shared_ptr<ValidationProtocolVStaging>,
                getValidationProtocolVStaging,
                (),
                (const, override));

    MOCK_METHOD(std::shared_ptr<BlockAnnounceProtocol>,
                getBlockAnnounceProtocol,
                (),
                (const, override));

    MOCK_METHOD(std::shared_ptr<CollationProtocolVStaging>,
                getCollationProtocolVStaging,
                (),
                (const, override));

    MOCK_METHOD(std::shared_ptr<CollationProtocol>,
                getCollationProtocol,
                (),
                (const, override));

    MOCK_METHOD(std::shared_ptr<ReqCollationProtocol>,
                getReqCollationProtocol,
                (),
                (const, override));

    MOCK_METHOD(std::shared_ptr<ReqPovProtocol>,
                getReqPovProtocol,
                (),
                (const, override));

    MOCK_METHOD(std::shared_ptr<FetchChunkProtocol>,
                getFetchChunkProtocol,
                (),
                (const, override));

    MOCK_METHOD(std::shared_ptr<FetchChunkProtocolObsolete>,
                getFetchChunkProtocolObsolete,
                (),
                (const, override));

    MOCK_METHOD(std::shared_ptr<FetchAttestedCandidateProtocol>,
                getFetchAttestedCandidateProtocol,
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

    MOCK_METHOD(std::shared_ptr<SendDisputeProtocol>,
                getSendDisputeProtocol,
                (),
                (const, override));

    MOCK_METHOD(std::shared_ptr<BeefyProtocol>,
                getBeefyProtocol,
                (),
                (const, override));

    MOCK_METHOD(std::shared_ptr<libp2p::protocol::Ping>,
                getPingProtocol,
                (),
                (const, override));
  };
}  // namespace kagome::network
