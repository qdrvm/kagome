/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/router_libp2p.hpp"

#include <libp2p/connection/stream.hpp>
#include <libp2p/protocol/ping.hpp>
#include "common/main_thread_pool.hpp"
#include "network/impl/protocols/beefy_justification_protocol.hpp"
#include "network/impl/protocols/beefy_protocol_impl.hpp"
#include "network/impl/protocols/block_announce_protocol.hpp"
#include "network/impl/protocols/grandpa_protocol.hpp"
#include "network/impl/protocols/light.hpp"
#include "network/impl/protocols/parachain_protocols.hpp"
#include "network/impl/protocols/propagate_transactions_protocol.hpp"
#include "network/impl/protocols/protocol_fetch_available_data.hpp"
#include "network/impl/protocols/protocol_fetch_chunk.hpp"
#include "network/impl/protocols/protocol_req_collation.hpp"
#include "network/impl/protocols/protocol_req_pov.hpp"
#include "network/impl/protocols/send_dispute_protocol.hpp"
#include "network/protocols/beefy_protocol.hpp"
#include "network/protocols/state_protocol.hpp"
#include "network/protocols/sync_protocol.hpp"
#include "network/types/bootstrap_nodes.hpp"
#include "network/warp/protocol.hpp"

namespace kagome::network {
  RouterLibp2p::RouterLibp2p(
      std::shared_ptr<application::AppStateManager> app_state_manager,
      std::shared_ptr<common::MainPoolHandler> main_pool_handler,
      libp2p::Host &host,
      const application::AppConfiguration &app_config,
      const OwnPeerInfo &own_info,
      const BootstrapNodes &bootstrap_nodes,
      LazySPtr<BlockAnnounceProtocol> block_announce_protocol,
      LazySPtr<GrandpaProtocol> grandpa_protocol,
      LazySPtr<SyncProtocol> sync_protocol,
      LazySPtr<StateProtocol> state_protocol,
      LazySPtr<WarpProtocol> warp_protocol,
      LazySPtr<BeefyProtocol> beefy_protocol,
      LazySPtr<BeefyJustificationProtocol> beefy_justifications_protocol,
      LazySPtr<LightProtocol> light_protocol,
      LazySPtr<PropagateTransactionsProtocol> propagate_transactions_protocol,
      LazySPtr<ValidationProtocol> validation_protocol,
      LazySPtr<CollationProtocol> collation_protocol,
      LazySPtr<ReqCollationProtocol> req_collation_protocol,
      LazySPtr<ReqPovProtocol> req_pov_protocol,
      LazySPtr<FetchChunkProtocol> fetch_chunk_protocol,
      LazySPtr<FetchAvailableDataProtocol> fetch_available_data_protocol,
      LazySPtr<StatementFetchingProtocol> statement_fetching_protocol,
      LazySPtr<SendDisputeProtocol> send_dispute_protocol,
      LazySPtr<libp2p::protocol::Ping> ping_protocol)
      : app_state_manager_{app_state_manager},
        host_{host},
        app_config_(app_config),
        own_info_{own_info},
        main_pool_handler_(std::move(main_pool_handler)),
        block_announce_protocol_(std::move(block_announce_protocol)),
        grandpa_protocol_(std::move(grandpa_protocol)),
        sync_protocol_(std::move(sync_protocol)),
        state_protocol_(std::move(state_protocol)),
        warp_protocol_{std::move(warp_protocol)},
        beefy_protocol_{std::move(beefy_protocol)},
        beefy_justifications_protocol_{
            std::move(beefy_justifications_protocol)},
        light_protocol_{std::move(light_protocol)},
        propagate_transactions_protocol_(
            std::move(propagate_transactions_protocol)),
        validation_protocol_(std::move(validation_protocol)),
        collation_protocol_(std::move(collation_protocol)),
        req_collation_protocol_(std::move(req_collation_protocol)),
        req_pov_protocol_(std::move(req_pov_protocol)),
        fetch_chunk_protocol_(std::move(fetch_chunk_protocol)),
        fetch_available_data_protocol_(
            std::move(fetch_available_data_protocol)),
        statement_fetching_protocol_(std::move(statement_fetching_protocol)),
        send_dispute_protocol_(std::move(send_dispute_protocol)),
        ping_protocol_{std::move(ping_protocol)},
        log_{log::createLogger("RouterLibp2p", "network")} {
    BOOST_ASSERT(app_state_manager_ != nullptr);
    BOOST_ASSERT(main_pool_handler_ != nullptr);

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
    main_pool_handler_->execute([wp = weak_from_this()] {
      if (auto self = wp.lock()) {
        self->block_announce_protocol_.get()->start();
        self->grandpa_protocol_.get()->start();

        self->sync_protocol_.get()->start();
        self->state_protocol_.get()->start();
        self->warp_protocol_.get()->start();
        self->beefy_protocol_.get()->start();
        self->beefy_justifications_protocol_.get()->start();
        self->light_protocol_.get()->start();

        self->propagate_transactions_protocol_.get()->start();

        self->collation_protocol_.get()->start();
        self->validation_protocol_.get()->start();
        self->req_collation_protocol_.get()->start();
        self->req_pov_protocol_.get()->start();
        self->fetch_chunk_protocol_.get()->start();
        self->fetch_available_data_protocol_.get()->start();
        self->statement_fetching_protocol_.get()->start();

        self->send_dispute_protocol_.get()->start();
      }
    });

    host_.setProtocolHandler(
        {ping_protocol_.get()->getProtocolId()},
        [wp{weak_from_this()}](auto &&stream_and_proto) {
          if (auto self = wp.lock()) {
            auto &stream = stream_and_proto.stream;
            if (auto peer_id = stream->remotePeerId()) {
              SL_TRACE(self->log_,
                       "Handled {} protocol stream from {}",
                       self->ping_protocol_.get()->getProtocolId(),
                       peer_id.value().toBase58());
              self->ping_protocol_.get()->handle(
                  std::forward<decltype(stream_and_proto)>(stream_and_proto));
            }
          }
        });

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
    return block_announce_protocol_.get();
  }

  std::shared_ptr<GrandpaProtocol> RouterLibp2p::getGrandpaProtocol() const {
    return grandpa_protocol_.get();
  }

  std::shared_ptr<SyncProtocol> RouterLibp2p::getSyncProtocol() const {
    return sync_protocol_.get();
  }

  std::shared_ptr<StateProtocol> RouterLibp2p::getStateProtocol() const {
    return state_protocol_.get();
  }

  std::shared_ptr<PropagateTransactionsProtocol>
  RouterLibp2p::getPropagateTransactionsProtocol() const {
    return propagate_transactions_protocol_.get();
  }

  std::shared_ptr<CollationProtocol> RouterLibp2p::getCollationProtocol()
      const {
    return collation_protocol_.get();
  }

  std::shared_ptr<ValidationProtocol> RouterLibp2p::getValidationProtocol()
      const {
    return validation_protocol_.get();
  }

  std::shared_ptr<ReqCollationProtocol> RouterLibp2p::getReqCollationProtocol()
      const {
    return req_collation_protocol_.get();
  }

  std::shared_ptr<ReqPovProtocol> RouterLibp2p::getReqPovProtocol() const {
    return req_pov_protocol_.get();
  }

  std::shared_ptr<FetchChunkProtocol> RouterLibp2p::getFetchChunkProtocol()
      const {
    return fetch_chunk_protocol_.get();
  }

  std::shared_ptr<FetchAvailableDataProtocol>
  RouterLibp2p::getFetchAvailableDataProtocol() const {
    return fetch_available_data_protocol_.get();
  }

  std::shared_ptr<StatementFetchingProtocol>
  RouterLibp2p::getFetchStatementProtocol() const {
    return statement_fetching_protocol_.get();
  }

  std::shared_ptr<SendDisputeProtocol> RouterLibp2p::getSendDisputeProtocol()
      const {
    return send_dispute_protocol_.get();
  }

  std::shared_ptr<BeefyProtocol> RouterLibp2p::getBeefyProtocol() const {
    return beefy_protocol_.get();
  }

  std::shared_ptr<libp2p::protocol::Ping> RouterLibp2p::getPingProtocol()
      const {
    return ping_protocol_.get();
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
