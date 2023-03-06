/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/protocols/protocol_factory.hpp"
#include "primitives/common.hpp"

namespace kagome::network {

  ProtocolFactory::ProtocolFactory(
      libp2p::Host &host,
      const application::AppConfiguration &app_config,
      const application::ChainSpec &chain_spec,
      const OwnPeerInfo &own_info,
      std::shared_ptr<boost::asio::io_context> io_context,
      std::shared_ptr<crypto::Hasher> hasher,
      std::shared_ptr<StreamEngine> stream_engine,
      std::shared_ptr<primitives::events::ExtrinsicSubscriptionEngine>
          extrinsic_events_engine,
      std::shared_ptr<subscription::ExtrinsicEventKeyRepository>
          ext_event_key_repo,
      std::shared_ptr<libp2p::basic::Scheduler> scheduler,
      std::shared_ptr<network::PeerView> peer_view,
      std::shared_ptr<ReputationRepository> reputation_repository)
      : host_(host),
        app_config_(app_config),
        chain_spec_(chain_spec),
        own_info_(own_info),
        io_context_(std::move(io_context)),
        hasher_(std::move(hasher)),
        stream_engine_(std::move(stream_engine)),
        extrinsic_events_engine_{std::move(extrinsic_events_engine)},
        ext_event_key_repo_{std::move(ext_event_key_repo)},
        reputation_repository_{std::move(reputation_repository)},
        scheduler_{std::move(scheduler)},
        peer_view_(std::move(peer_view)) {
    BOOST_ASSERT(io_context_ != nullptr);
    BOOST_ASSERT(hasher_ != nullptr);
    BOOST_ASSERT(stream_engine_ != nullptr);
    BOOST_ASSERT(extrinsic_events_engine_ != nullptr);
    BOOST_ASSERT(ext_event_key_repo_ != nullptr);
    BOOST_ASSERT(reputation_repository_ != nullptr);
    BOOST_ASSERT(scheduler_ != nullptr);
    BOOST_ASSERT(peer_view_);
  }

  std::shared_ptr<BlockAnnounceProtocol>
  ProtocolFactory::makeBlockAnnounceProtocol() const {
    auto block_tree = block_tree_.lock();
    BOOST_ASSERT(block_tree != nullptr);
    auto genesisBlockHash = block_tree->getGenesisBlockHash();

    return std::make_shared<BlockAnnounceProtocol>(host_,
                                                   app_config_,
                                                   chain_spec_,
                                                   genesisBlockHash,
                                                   stream_engine_,
                                                   std::move(block_tree),
                                                   babe_.lock(),
                                                   peer_manager_.lock());
  }

  std::shared_ptr<GrandpaProtocol> ProtocolFactory::makeGrandpaProtocol()
      const {
    auto block_tree = block_tree_.lock();
    BOOST_ASSERT(block_tree != nullptr);
    auto genesisBlockHash = block_tree->getGenesisBlockHash();

    return std::make_shared<GrandpaProtocol>(host_,
                                             io_context_,
                                             app_config_,
                                             grandpa_observer_.lock(),
                                             own_info_,
                                             stream_engine_,
                                             peer_manager_.lock(),
                                             genesisBlockHash,
                                             scheduler_);
  }

  std::shared_ptr<ValidationProtocol> ProtocolFactory::makeValidationProtocol()
      const {
    auto block_tree = block_tree_.lock();
    BOOST_ASSERT(block_tree != nullptr);
    auto genesisBlockHash = block_tree->getGenesisBlockHash();

    return std::make_shared<ValidationProtocol>(host_,
                                                app_config_,
                                                chain_spec_,
                                                genesisBlockHash,
                                                validation_observer_.lock(),
                                                kValidationProtocol,
                                                peer_view_);
  }

  std::shared_ptr<CollationProtocol> ProtocolFactory::makeCollationProtocol()
      const {
    auto block_tree = block_tree_.lock();
    BOOST_ASSERT(block_tree != nullptr);
    auto genesisBlockHash = block_tree->getGenesisBlockHash();

    return std::make_shared<CollationProtocol>(host_,
                                               app_config_,
                                               chain_spec_,
                                               genesisBlockHash,
                                               collation_observer_.lock(),
                                               kCollationProtocol,
                                               peer_view_);
  }

  std::shared_ptr<ReqCollationProtocol>
  ProtocolFactory::makeReqCollationProtocol() const {
    auto block_tree = block_tree_.lock();
    BOOST_ASSERT(block_tree != nullptr);
    auto genesisBlockHash = block_tree->getGenesisBlockHash();

    return std::make_shared<ReqCollationProtocol>(
        host_, chain_spec_, genesisBlockHash, req_collation_observer_.lock());
  }

  std::shared_ptr<ReqPovProtocol> ProtocolFactory::makeReqPovProtocol() const {
    auto block_tree = block_tree_.lock();
    BOOST_ASSERT(block_tree != nullptr);
    auto genesisBlockHash = block_tree->getGenesisBlockHash();

    return std::make_shared<ReqPovProtocol>(
        host_, chain_spec_, genesisBlockHash, req_pov_observer_.lock());
  }

  std::shared_ptr<FetchChunkProtocol> ProtocolFactory::makeFetchChunkProtocol()
      const {
    auto block_tree = block_tree_.lock();
    BOOST_ASSERT(block_tree != nullptr);
    auto genesisBlockHash = block_tree->getGenesisBlockHash();

    auto pp = parachain_processor_.lock();
    BOOST_ASSERT(pp);

    return std::make_shared<FetchChunkProtocol>(
        host_, chain_spec_, genesisBlockHash, pp);
  }

  std::shared_ptr<FetchAvailableDataProtocol>
  ProtocolFactory::makeFetchAvailableDataProtocol() const {
    auto block_tree = block_tree_.lock();
    BOOST_ASSERT(block_tree != nullptr);
    auto genesisBlockHash = block_tree->getGenesisBlockHash();

    auto pp = parachain_processor_.lock();
    BOOST_ASSERT(pp);

    return std::make_shared<FetchAvailableDataProtocol>(
        host_, chain_spec_, genesisBlockHash, pp->getAvStore());
  }

  std::shared_ptr<StatmentFetchingProtocol>
  ProtocolFactory::makeFetchStatementProtocol() const {
    auto block_tree = block_tree_.lock();
    BOOST_ASSERT(block_tree != nullptr);
    auto genesisBlockHash = block_tree->getGenesisBlockHash();

    auto pp = parachain_processor_.lock();
    BOOST_ASSERT(pp);

    return std::make_shared<StatmentFetchingProtocol>(
        host_, chain_spec_, genesisBlockHash, pp->getBackingStore());
  }

  std::shared_ptr<PropagateTransactionsProtocol>
  ProtocolFactory::makePropagateTransactionsProtocol() const {
    auto block_tree = block_tree_.lock();
    BOOST_ASSERT(block_tree != nullptr);
    auto genesisBlockHash = block_tree->getGenesisBlockHash();

    return std::make_shared<PropagateTransactionsProtocol>(
        host_,
        app_config_,
        chain_spec_,
        genesisBlockHash,
        babe_.lock(),
        extrinsic_observer_.lock(),
        stream_engine_,
        extrinsic_events_engine_,
        ext_event_key_repo_);
  }

  std::shared_ptr<StateProtocol> ProtocolFactory::makeStateProtocol() const {
    auto block_tree = block_tree_.lock();
    BOOST_ASSERT(block_tree != nullptr);
    auto genesisBlockHash = block_tree->getGenesisBlockHash();

    return std::make_shared<StateProtocolImpl>(
        host_, chain_spec_, genesisBlockHash, state_observer_.lock());
  }

  std::shared_ptr<SyncProtocol> ProtocolFactory::makeSyncProtocol() const {
    auto block_tree = block_tree_.lock();
    BOOST_ASSERT(block_tree != nullptr);
    auto genesisBlockHash = block_tree->getGenesisBlockHash();

    return std::make_shared<SyncProtocolImpl>(host_,
                                              chain_spec_,
                                              genesisBlockHash,
                                              sync_observer_.lock(),
                                              reputation_repository_);
  }

}  // namespace kagome::network
