/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "protocol_factory.hpp"

namespace kagome::network {

  ProtocolFactory::ProtocolFactory(
      libp2p::Host &host,
      const application::ChainSpec &chain_spec,
      const OwnPeerInfo &own_info,
      std::shared_ptr<blockchain::BlockStorage> storage,
      std::shared_ptr<crypto::Hasher> hasher,
      std::shared_ptr<StreamEngine> stream_engine)
      : host_(host),
        chain_spec_(chain_spec),
        own_info_(own_info),
        storage_(std::move(storage)),
        hasher_(std::move(hasher)),
        stream_engine_(std::move(stream_engine)) {
    BOOST_ASSERT(storage_ != nullptr);
    BOOST_ASSERT(hasher_ != nullptr);
    BOOST_ASSERT(stream_engine_ != nullptr);
  }

  std::shared_ptr<BlockAnnounceProtocol>
  ProtocolFactory::makeBlockAnnounceProtocol() const {
    return std::make_shared<BlockAnnounceProtocol>(host_,
                                                   chain_spec_,
                                                   stream_engine_,
                                                   block_tree_.lock(),
                                                   storage_,
                                                   babe_observer_.lock(),
                                                   hasher_,
                                                   peer_manager_.lock());
  }

  std::shared_ptr<GossipProtocol> ProtocolFactory::makeGossipProtocol() const {
    return std::make_shared<GossipProtocol>(
        host_, grandpa_observer_.lock(), own_info_, stream_engine_);
  }

  std::shared_ptr<GrandpaProtocol> ProtocolFactory::makeGrandpaProtocol()
      const {
    return std::make_shared<GrandpaProtocol>(host_, stream_engine_);
  }

  std::shared_ptr<PropagateTransactionsProtocol>
  ProtocolFactory::makePropagateTransactionsProtocol() const {
    return std::make_shared<PropagateTransactionsProtocol>(
        host_, chain_spec_, extrinsic_observer_.lock(), stream_engine_);
  }

  std::shared_ptr<SupProtocol> ProtocolFactory::makeSupProtocol() const {
    return std::make_shared<SupProtocol>(host_, block_tree_.lock(), storage_);
  }

  std::shared_ptr<SyncProtocol> ProtocolFactory::makeSyncProtocol() const {
    return std::make_shared<SyncProtocol>(
        host_, chain_spec_, sync_observer_.lock());
  }

}  // namespace kagome::network
