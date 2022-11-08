/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/router_libp2p.hpp"

namespace kagome::network {
  RouterLibp2p::RouterLibp2p(
      std::shared_ptr<application::AppStateManager> app_state_manager,
      libp2p::Host &host,
      const application::AppConfiguration &app_config,
      const OwnPeerInfo &own_info,
      const BootstrapNodes &bootstrap_nodes,
      std::shared_ptr<libp2p::protocol::Ping> ping_proto,
      std::shared_ptr<network::ProtocolFactory> protocol_factory)
      : app_state_manager_{app_state_manager},
        host_{host},
        app_config_(app_config),
        own_info_{own_info},
        log_{log::createLogger("RouterLibp2p", "network")},
        ping_protocol_{std::move(ping_proto)},
        protocol_factory_{std::move(protocol_factory)} {
    BOOST_ASSERT(app_state_manager_ != nullptr);
    BOOST_ASSERT(ping_protocol_ != nullptr);
    BOOST_ASSERT(protocol_factory_ != nullptr);

    SL_DEBUG(log_, "Own peer id: {}", own_info.id.toBase58());
    if (!bootstrap_nodes.empty()) {
      for (const auto &peer_info : bootstrap_nodes) {
        for (auto &addr : peer_info.addresses) {
          SL_DEBUG(log_, "Bootstrap node: {}", addr.getStringAddress());
        }
      }
    } else if (app_config_.isRunInDevMode()) {
      SL_DEBUG(log_, "No bootstrap node. Dev mode.");
    } else {
      log_->error("No bootstrap node");
    }

    app_state_manager_->takeControl(*this);
  }

  bool RouterLibp2p::prepare() {
    host_.setProtocolHandler(
        {ping_protocol_->getProtocolId()},
        [wp = weak_from_this()](auto &&stream_and_proto) {
          if (auto self = wp.lock()) {
            auto &stream = stream_and_proto.stream;
            if (auto peer_id = stream->remotePeerId()) {
              SL_TRACE(self->log_,
                       "Handled {} protocol stream from: {}",
                       self->ping_protocol_->getProtocolId(),
                       peer_id.value().toBase58());
              self->ping_protocol_->handle(
                  std::forward<decltype(stream_and_proto)>(stream_and_proto));
            }
          }
        });

    block_announce_protocol_ = protocol_factory_->makeBlockAnnounceProtocol();
    if (not block_announce_protocol_) {
      return false;
    }

    collation_protocol_ = protocol_factory_->makeCollationProtocol();
    if (not collation_protocol_) {
      return false;
    }

    validation_protocol_ = protocol_factory_->makeValidationProtocol();
    if (not validation_protocol_) {
      return false;
    }

    req_collation_protocol_ = protocol_factory_->makeReqCollationProtocol();
    if (not req_collation_protocol_) {
      return false;
    }

    grandpa_protocol_ = protocol_factory_->makeGrandpaProtocol();
    if (not grandpa_protocol_) {
      return false;
    }

    propagate_transaction_protocol_ =
        protocol_factory_->makePropagateTransactionsProtocol();
    if (not propagate_transaction_protocol_) {
      return false;
    }

    state_protocol_ = protocol_factory_->makeStateProtocol();
    if (not state_protocol_) {
      return false;
    }

    sync_protocol_ = protocol_factory_->makeSyncProtocol();
    if (not sync_protocol_) {
      return false;
    }

    block_announce_protocol_->start();
    grandpa_protocol_->start();
    propagate_transaction_protocol_->start();
    state_protocol_->start();
    sync_protocol_->start();
    collation_protocol_->start();
    validation_protocol_->start();
    req_collation_protocol_->start();

    return true;
  }

  bool RouterLibp2p::start() {
    auto listen_addresses = app_config_.listenAddresses();
    for (auto &listen_address : listen_addresses) {
      // fully formatted listen address is used inside Kademlia
      auto append_res = appendPeerIdToAddress(listen_address);
      if (not append_res) {
        log_->error("Cannot append peer id info to listen addr {}. Error: {}",
                    listen_address.getStringAddress(),
                    append_res.error());
        // despite the possible failure of address reformatting we still
        // intentionally try to start listening the interface
      }
      auto res = host_.listen(listen_address);
      if (not res) {
        log_->error("Cannot listen on address {}. Error: {}",
                    listen_address.getStringAddress(),
                    res.error());
      }
    }

    auto &addr_repo = host_.getPeerRepository().getAddressRepository();
    // here we put our known public addresses to the repository
    auto upsert_res = addr_repo.upsertAddresses(
        own_info_.id, own_info_.addresses, libp2p::peer::ttl::kPermanent);
    if (!upsert_res) {
      log_->error("Cannot add own addresses to repo: {}", upsert_res.error());
    }

    host_.start();

    const auto &host_addresses = host_.getAddresses();
    if (host_addresses.empty()) {
      log_->critical("Host addresses is empty");
      return false;
    }

    log_->info("Started with peer id: {}", host_.getId().toBase58());
    for (const auto &addr : host_addresses) {
      log_->info("Started listening on address: {}", addr.getStringAddress());
    }

    return true;
  }

  void RouterLibp2p::stop() {
    if (host_.getNetwork().getListener().isStarted()) {
      host_.stop();
    }
  }

  std::shared_ptr<BlockAnnounceProtocol>
  RouterLibp2p::getBlockAnnounceProtocol() const {
    return block_announce_protocol_;
  }

  std::shared_ptr<CollationProtocol> RouterLibp2p::getCollationProtocol()
      const {
    return collation_protocol_;
  }

  std::shared_ptr<ValidationProtocol> RouterLibp2p::getValidationProtocol()
      const {
    return validation_protocol_;
  }

  std::shared_ptr<ReqCollationProtocol> RouterLibp2p::getReqCollationProtocol()
      const {
    return req_collation_protocol_;
  }

  std::shared_ptr<PropagateTransactionsProtocol>
  RouterLibp2p::getPropagateTransactionsProtocol() const {
    return propagate_transaction_protocol_;
  }

  std::shared_ptr<StateProtocol> RouterLibp2p::getStateProtocol() const {
    return state_protocol_;
  }

  std::shared_ptr<SyncProtocol> RouterLibp2p::getSyncProtocol() const {
    return sync_protocol_;
  }

  std::shared_ptr<GrandpaProtocol> RouterLibp2p::getGrandpaProtocol() const {
    return grandpa_protocol_;
  }

  std::shared_ptr<libp2p::protocol::Ping> RouterLibp2p::getPingProtocol()
      const {
    return ping_protocol_;
  }

  outcome::result<void> RouterLibp2p::appendPeerIdToAddress(
      libp2p::multi::Multiaddress &address) const {
    using P = libp2p::multi::Protocol::Code;

    if (address.getProtocols().size()
            < 3  // does not have /p2p or alternative part
        and (address.hasProtocol(P::IP4) or address.hasProtocol(P::IP6))
        and address.hasProtocol(P::TCP)) {
      // PeerId is already initialized in both cases - when it is predefined or
      // autogenerated

      auto ma_res = libp2p::multi::Multiaddress::create(
          std::string(address.getStringAddress()) + "/p2p/"
          + own_info_.id.toBase58());
      if (not ma_res) {
        return ma_res.error();
      }
      address = ma_res.value();
    }
    return outcome::success();
  }

}  // namespace kagome::network
