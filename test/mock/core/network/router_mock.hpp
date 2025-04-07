/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "network/router.hpp"

#include <gmock/gmock.h>

#include "mock/core/network/protocol_mocks.hpp"
#include "mock/core/network/protocols/req_collation_protocol_mock.hpp"
#include "mock/core/network/protocols/req_pov_protocol_mock.hpp"

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
    auto getMockedFetchAvailableDataProtocol() const {
      return fetch_available_data;
    }

    MOCK_METHOD(std::shared_ptr<ValidationProtocol>,
                getValidationProtocol,
                (),
                (const, override));

    MOCK_METHOD(std::shared_ptr<BlockAnnounceProtocol>,
                getBlockAnnounceProtocol,
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
    auto getMockedFetchChunkProtocol() const {
      return fetch_chunk;
    }

    MOCK_METHOD(std::shared_ptr<FetchChunkProtocolObsolete>,
                getFetchChunkProtocolObsolete,
                (),
                (const, override));
    auto getMockedFetchChunkProtocolObsolete() const {
      return fetch_chunk_obsolete;
    }

    MOCK_METHOD(std::shared_ptr<FetchAttestedCandidateProtocol>,
                getFetchAttestedCandidateProtocol,
                (),
                (const, override));

    MOCK_METHOD(std::shared_ptr<PropagateTransactionsProtocol>,
                getPropagateTransactionsProtocol,
                (),
                (const, override));

    MOCK_METHOD(std::shared_ptr<WarpProtocol>,
                getWarpProtocol,
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

    MOCK_METHOD(std::shared_ptr<Ping>, getPingProtocol, (), (const, override));

    void setReturningMockedProtocols() {
      // clang-format off
      // ON_CALL(*this, getBlockAnnounceProtocol()).WillByDefault(testing::Return(block_announce));
      // ON_CALL(*this, getGrandpaProtocol()).WillByDefault(testing::Return(grandpa));
      // ON_CALL(*this, getSyncProtocol()).WillByDefault(testing::Return(sync));
      // ON_CALL(*this, getStateProtocol()).WillByDefault(testing::Return(state));
      // ON_CALL(*this, getWarpProtocol()).WillByDefault(testing::Return(warp));
      // ON_CALL(*this, getBeefyProtocol()).WillByDefault(testing::Return(beefy));
      // ON_CALL(*this, getBeefyJustificationProtocol()).WillByDefault(testing::Return(beefy_justifications));
      // ON_CALL(*this, getLightProtocol()).WillByDefault(testing::Return(light));
      // ON_CALL(*this, getPropagateTransactionsProtocol()).WillByDefault(testing::Return(propagate_transactions));
      // ON_CALL(*this, getValidationProtocol()).WillByDefault(testing::Return(validation));
      // ON_CALL(*this, getCollationProtocol()).WillByDefault(testing::Return(collation));
      // ON_CALL(*this, getReqCollationProtocol()).WillByDefault(testing::Return(req_collation));
      // ON_CALL(*this, getReqPovProtocol()).WillByDefault(testing::Return(req_pov));
      fetch_chunk = std::make_shared<FetchChunkProtocolMock>();
      ON_CALL(*this, getFetchChunkProtocol()).WillByDefault(testing::Return(fetch_chunk));
      fetch_chunk_obsolete = std::make_shared<FetchChunkProtocolObsoleteMock>();
      ON_CALL(*this, getFetchChunkProtocolObsolete()).WillByDefault(testing::Return(fetch_chunk_obsolete));
      fetch_available_data = std::make_shared<FetchAvailableDataProtocolMock>();
      ON_CALL(*this, getFetchAvailableDataProtocol()).WillByDefault(testing::Return(fetch_available_data));
      // ON_CALL(*this, getStatementFetchingProtocol()).WillByDefault(testing::Return(statement_fetching));
      // ON_CALL(*this, getSendDisputeProtocol()).WillByDefault(testing::Return(send_dispute));
      // ON_CALL(*this, getPing()).WillByDefault(testing::Return(ping));
      // ON_CALL(*this, getFetchAttestedCandidateProtocol()).WillByDefault(testing::Return(fetch_attested_candidate));
      // clang-format on
    }

   private:
    // clang-format off
    // std::shared_ptr<BlockAnnounceProtocolMock> block_announce;
    // std::shared_ptr<GrandpaProtocolMock> grandpa;
    // std::shared_ptr<SyncProtocolMock> sync;
    // std::shared_ptr<StateProtocolMock> state;
    // std::shared_ptr<WarpProtocolMock> warp;
    // std::shared_ptr<BeefyProtocolMock> beefy;
    // std::shared_ptr<BeefyJustificationProtocolMock> beefy_justifications;
    // std::shared_ptr<LightProtocolMock> light;
    // std::shared_ptr<PropagateTransactionsProtocolMock> propagate_transactions;
    // std::shared_ptr<ValidationProtocolMock> validation;
    // std::shared_ptr<CollationProtocolMock> collation;
    // std::shared_ptr<CollationProtocolVStagingMock> collation_vstaging;
    // std::shared_ptr<ValidationProtocolVStagingMock> validation_vstaging;
    // std::shared_ptr<ReqCollationProtocolMock> req_collation;
    // std::shared_ptr<ReqPovProtocolMock> req_pov;
    std::shared_ptr<FetchChunkProtocolMock> fetch_chunk;
    std::shared_ptr<FetchChunkProtocolObsoleteMock> fetch_chunk_obsolete;
    std::shared_ptr<FetchAvailableDataProtocolMock> fetch_available_data;
    // std::shared_ptr<StatementFetchingProtocolMock> statement_fetching;
    // std::shared_ptr<SendDisputeProtocolMock> send_dispute;
    // std::shared_ptr<PingMock> ping;
    // std::shared_ptr<FetchAttestedCandidateProtocolMock> fetch_attested_candidate;
    // clang-format on
  };
}  // namespace kagome::network
