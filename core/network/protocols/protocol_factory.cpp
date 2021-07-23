/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "protocol_factory.hpp"

namespace kagome::network {

  ProtocolFactory::ProtocolFactory(
      libp2p::Host &host,
      const application::AppConfiguration &app_config,
      const application::ChainSpec &chain_spec,
      const OwnPeerInfo &own_info,
      std::shared_ptr<boost::asio::io_context> io_context,
      std::shared_ptr<blockchain::BlockStorage> storage,
      std::shared_ptr<crypto::Hasher> hasher,
      std::shared_ptr<StreamEngine> stream_engine,
      std::shared_ptr<primitives::events::ExtrinsicSubscriptionEngine>
          extrinsic_events_engine,
      std::shared_ptr<subscription::ExtrinsicEventKeyRepository>
          ext_event_key_repo)
      : host_(host),
        app_config_(app_config),
        chain_spec_(chain_spec),
        own_info_(own_info),
        io_context_(std::move(io_context)),
        storage_(std::move(storage)),
        hasher_(std::move(hasher)),
        stream_engine_(std::move(stream_engine)),
        extrinsic_events_engine_{std::move(extrinsic_events_engine)},
        ext_event_key_repo_{std::move(ext_event_key_repo)} {
    BOOST_ASSERT(io_context_ != nullptr);
    BOOST_ASSERT(storage_ != nullptr);
    BOOST_ASSERT(hasher_ != nullptr);
    BOOST_ASSERT(stream_engine_ != nullptr);
    BOOST_ASSERT(extrinsic_events_engine_ != nullptr);
    BOOST_ASSERT(ext_event_key_repo_ != nullptr);
  }

  std::shared_ptr<BlockAnnounceProtocol>
  ProtocolFactory::makeBlockAnnounceProtocol() const {
    return std::make_shared<BlockAnnounceProtocol>(host_,
                                                   app_config_,
                                                   chain_spec_,
                                                   stream_engine_,
                                                   block_tree_.lock(),
                                                   storage_,
                                                   babe_.lock(),
                                                   hasher_,
                                                   peer_manager_.lock());
  }

  std::shared_ptr<GrandpaProtocol> ProtocolFactory::makeGrandpaProtocol()
      const {
    return std::make_shared<GrandpaProtocol>(host_,
                                             io_context_,
                                             app_config_,
                                             grandpa_observer_.lock(),
                                             own_info_,
                                             stream_engine_);
  }

  std::shared_ptr<PropagateTransactionsProtocol>
  ProtocolFactory::makePropagateTransactionsProtocol() const {
    return std::make_shared<PropagateTransactionsProtocol>(
        host_,
        chain_spec_,
        babe_.lock(),
        extrinsic_observer_.lock(),
        stream_engine_,
        extrinsic_events_engine_,
        ext_event_key_repo_);
  }

  std::shared_ptr<SyncProtocol> ProtocolFactory::makeSyncProtocol() const {
    return std::make_shared<SyncProtocol>(
        host_, chain_spec_, sync_observer_.lock());
  }

}  // namespace kagome::network
