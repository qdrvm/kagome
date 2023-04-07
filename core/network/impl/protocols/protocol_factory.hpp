/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_PROTOCOLFACTORY
#define KAGOME_NETWORK_PROTOCOLFACTORY

#include <libp2p/basic/scheduler.hpp>

#include "application/app_configuration.hpp"
#include "consensus/babe/babe.hpp"
#include "network/impl/protocols/block_announce_protocol.hpp"
#include "network/impl/protocols/grandpa_protocol.hpp"
#include "network/impl/protocols/parachain_protocols.hpp"
#include "network/impl/protocols/propagate_transactions_protocol.hpp"
#include "network/impl/protocols/protocol_fetch_available_data.hpp"
#include "network/impl/protocols/protocol_fetch_chunk.hpp"
#include "network/impl/protocols/protocol_req_collation.hpp"
#include "network/impl/protocols/protocol_req_pov.hpp"
#include "network/impl/protocols/request_response_protocol.hpp"
#include "network/impl/protocols/state_protocol_impl.hpp"
#include "network/impl/protocols/sync_protocol_impl.hpp"
#include "network/impl/stream_engine.hpp"
#include "network/reputation_repository.hpp"
#include "parachain/validator/parachain_observer.hpp"
#include "primitives/event_types.hpp"

namespace kagome::blockchain {
  class GenesisBlockHash;
}

namespace kagome::network {

  class ProtocolFactory final {
   public:
    ProtocolFactory(
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
        LazySPtr<StatmentFetchingProtocol> statement_fetching_protocol);

    std::shared_ptr<BlockAnnounceProtocol> makeBlockAnnounceProtocol() const;

    std::shared_ptr<GrandpaProtocol> makeGrandpaProtocol() const;

    std::shared_ptr<PropagateTransactionsProtocol>
    makePropagateTransactionsProtocol() const;

    std::shared_ptr<StateProtocol> makeStateProtocol() const;
    std::shared_ptr<SyncProtocol> makeSyncProtocol() const;

    std::shared_ptr<CollationProtocol> makeCollationProtocol() const;
    std::shared_ptr<ValidationProtocol> makeValidationProtocol() const;
    std::shared_ptr<ReqCollationProtocol> makeReqCollationProtocol() const;
    std::shared_ptr<ReqPovProtocol> makeReqPovProtocol() const;
    std::shared_ptr<FetchChunkProtocol> makeFetchChunkProtocol() const;
    std::shared_ptr<FetchAvailableDataProtocol> makeFetchAvailableDataProtocol()
        const;
    std::shared_ptr<StatmentFetchingProtocol> makeFetchStatementProtocol()
        const;

   private:
    LazySPtr<BlockAnnounceProtocol> block_announce_protocol_;
    LazySPtr<GrandpaProtocol> grandpa_protocol_;
    LazySPtr<SyncProtocol> sync_protocol_;
    LazySPtr<StateProtocol> state_protocol_;
    LazySPtr<PropagateTransactionsProtocol> propagate_transactions_protocol_;
    LazySPtr<ValidationProtocol> validation_protocol_;
    LazySPtr<CollationProtocol> collation_protocol_;
    LazySPtr<ReqCollationProtocol> req_collation_protocol_;
    LazySPtr<ReqPovProtocol> req_pov_protocol_;
    LazySPtr<FetchChunkProtocol> fetch_chunk_protocol_;
    LazySPtr<FetchAvailableDataProtocol> fetch_available_data_protocol_;
    LazySPtr<StatmentFetchingProtocol> statement_fetching_protocol_;
  };

}  // namespace kagome::network

#endif  // KAGOME_NETWORK_PROTOCOLFACTORY
