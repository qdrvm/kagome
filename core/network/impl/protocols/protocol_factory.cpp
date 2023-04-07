/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/protocols/protocol_factory.hpp"
#include "blockchain/genesis_block_hash.hpp"
#include "primitives/common.hpp"

namespace kagome::network {

  ProtocolFactory::ProtocolFactory(
      LazySPtr<BlockAnnounceProtocol> block_announce_protocol,
      LazySPtr<GrandpaProtocol> grandpa_protocol,
      LazySPtr<SyncProtocol> sync_protocol,
      LazySPtr<StateProtocol> state_protocol,
      LazySPtr<PropagateTransactionsProtocol> propagate_transactions_protocol,
      LazySPtr<ValidationProtocol> validation_protocol,
      LazySPtr<CollationProtocol> collation_protocol,
      LazySPtr<ReqCollationProtocol> req_collation_protocol,
      LazySPtr<ReqPovProtocol> req_pov_protocol,
      LazySPtr<FetchChunkProtocol> fetch_chunk_protocol,
      LazySPtr<FetchAvailableDataProtocol> fetch_available_data_protocol,
      LazySPtr<StatmentFetchingProtocol> statement_fetching_protocol)
      : block_announce_protocol_(std::move(block_announce_protocol)),
        grandpa_protocol_(std::move(grandpa_protocol)),
        sync_protocol_(std::move(sync_protocol)),
        state_protocol_(std::move(state_protocol)),
        propagate_transactions_protocol_(
            std::move(propagate_transactions_protocol)),
        validation_protocol_(std::move(validation_protocol)),
        collation_protocol_(std::move(collation_protocol)),
        req_collation_protocol_(std::move(req_collation_protocol)),
        req_pov_protocol_(std::move(req_pov_protocol)),
        fetch_chunk_protocol_(std::move(fetch_chunk_protocol)),
        fetch_available_data_protocol_(
            std::move(fetch_available_data_protocol)),
        statement_fetching_protocol_(std::move(statement_fetching_protocol)) {}

  std::shared_ptr<BlockAnnounceProtocol>
  ProtocolFactory::makeBlockAnnounceProtocol() const {
    return block_announce_protocol_.get();
  }

  std::shared_ptr<GrandpaProtocol> ProtocolFactory::makeGrandpaProtocol()
      const {
    return grandpa_protocol_.get();
  }

  std::shared_ptr<ValidationProtocol> ProtocolFactory::makeValidationProtocol()
      const {
    return validation_protocol_.get();
  }

  std::shared_ptr<CollationProtocol> ProtocolFactory::makeCollationProtocol()
      const {
    return collation_protocol_.get();
  }

  std::shared_ptr<ReqCollationProtocol>
  ProtocolFactory::makeReqCollationProtocol() const {
    return req_collation_protocol_.get();
  }

  std::shared_ptr<ReqPovProtocol> ProtocolFactory::makeReqPovProtocol() const {
    return req_pov_protocol_.get();
  }

  std::shared_ptr<FetchChunkProtocol> ProtocolFactory::makeFetchChunkProtocol()
      const {
    return fetch_chunk_protocol_.get();
  }

  std::shared_ptr<FetchAvailableDataProtocol>
  ProtocolFactory::makeFetchAvailableDataProtocol() const {
    return fetch_available_data_protocol_.get();
  }

  std::shared_ptr<StatmentFetchingProtocol>
  ProtocolFactory::makeFetchStatementProtocol() const {
    return statement_fetching_protocol_.get();
  }

  std::shared_ptr<PropagateTransactionsProtocol>
  ProtocolFactory::makePropagateTransactionsProtocol() const {
    return propagate_transactions_protocol_.get();
  }

  std::shared_ptr<StateProtocol> ProtocolFactory::makeStateProtocol() const {
    return state_protocol_.get();
  }

  std::shared_ptr<SyncProtocol> ProtocolFactory::makeSyncProtocol() const {
    return sync_protocol_.get();
  }

}  // namespace kagome::network
