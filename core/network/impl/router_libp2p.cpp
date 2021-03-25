/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/router_libp2p.hpp"

#include <libp2p/basic/message_read_writer_uvarint.hpp>

#include "application/chain_spec.hpp"
#include "blockchain/block_storage.hpp"
#include "consensus/grandpa/structs.hpp"
#include "network/adapters/protobuf_block_request.hpp"
#include "network/adapters/protobuf_block_response.hpp"
#include "network/common.hpp"
#include "network/helpers/protobuf_message_read_writer.hpp"
#include "network/rpc.hpp"
#include "network/types/block_announce.hpp"
#include "network/types/blocks_request.hpp"
#include "network/types/blocks_response.hpp"
#include "network/types/bootstrap_nodes.hpp"
#include "network/types/no_data_message.hpp"
#include "network/types/status.hpp"
#include "scale/scale.hpp"

namespace kagome::network {
  RouterLibp2p::RouterLibp2p(
      std::shared_ptr<application::AppStateManager> app_state_manager,
      libp2p::Host &host,
      const application::AppConfiguration &app_config,
      std::shared_ptr<application::ChainSpec> chain_spec,
      const OwnPeerInfo &own_info,
      std::shared_ptr<StreamEngine> stream_engine,
      std::shared_ptr<BabeObserver> babe_observer,
      std::shared_ptr<consensus::grandpa::GrandpaObserver> grandpa_observer,
      std::shared_ptr<SyncProtocolObserver> sync_observer,
      std::shared_ptr<ExtrinsicObserver> extrinsic_observer,
      std::shared_ptr<Gossiper> gossiper,
      const BootstrapNodes &bootstrap_nodes,
      std::shared_ptr<blockchain::BlockStorage> storage,
      std::shared_ptr<libp2p::protocol::Ping> ping_proto,
      std::shared_ptr<PeerManager> peer_manager,
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<crypto::Hasher> hasher,
      std::shared_ptr<BlockAnnounceProtocol> block_announce_protocol)
      : app_state_manager_{app_state_manager},
        host_{host},
        app_config_(app_config),
        chain_spec_(std::move(chain_spec)),
        own_info_{own_info},
        stream_engine_{std::move(stream_engine)},
        babe_observer_{std::move(babe_observer)},
        grandpa_observer_{std::move(grandpa_observer)},
        sync_observer_{std::move(sync_observer)},
        extrinsic_observer_{std::move(extrinsic_observer)},
        gossiper_{std::move(gossiper)},
        log_{common::createLogger("RouterLibp2p")},
        storage_{std::move(storage)},
        ping_proto_{std::move(ping_proto)},
        peer_manager_{std::move(peer_manager)},
        block_tree_{std::move(block_tree)},
        hasher_{std::move(hasher)},
        block_announce_protocol_{std::move(block_announce_protocol)} {
    BOOST_ASSERT_MSG(app_state_manager_ != nullptr,
                     "app state manager is nullptr");
    BOOST_ASSERT_MSG(stream_engine_ != nullptr, "stream engine is nullptr");
    BOOST_ASSERT_MSG(babe_observer_ != nullptr, "babe observer is nullptr");
    BOOST_ASSERT_MSG(grandpa_observer_ != nullptr,
                     "grandpa observer is nullptr");
    BOOST_ASSERT_MSG(sync_observer_ != nullptr, "sync observer is nullptr");
    BOOST_ASSERT_MSG(extrinsic_observer_ != nullptr,
                     "author api observer is nullptr");
    BOOST_ASSERT_MSG(gossiper_ != nullptr, "gossiper is nullptr");
    BOOST_ASSERT_MSG(!bootstrap_nodes.empty(), "bootstrap node list is empty");
    BOOST_ASSERT(storage_ != nullptr);
    BOOST_ASSERT(ping_proto_ != nullptr);
    BOOST_ASSERT(peer_manager_ != nullptr);
    BOOST_ASSERT(block_tree_ != nullptr);
    BOOST_ASSERT(hasher_ != nullptr);
    BOOST_ASSERT(block_announce_protocol_ != nullptr);

    log_->debug("Own peer id: {}", own_info.id.toBase58());
    for (const auto &peer_info : bootstrap_nodes) {
      for (auto &addr : peer_info.addresses) {
        log_->debug("Bootstrap node: {}", addr.getStringAddress());
      }
    }

    gossiper_->storeSelfPeerInfo(own_info_);

    auto stream = std::make_shared<LoopbackStream>(own_info_);
    loopback_stream_ = stream;
    [[maybe_unused]] auto res =
        stream_engine_->add(std::move(stream), kGossipProtocol);
    BOOST_ASSERT(res.has_value());

    app_state_manager_->takeControl(*this);
  }

  bool RouterLibp2p::prepare() {
    host_.setProtocolHandler(
        ping_proto_->getProtocolId(), [wp = weak_from_this()](auto &&stream) {
          if (auto self = wp.lock()) {
            if (auto peer_id = stream->remotePeerId()) {
              self->log_->info("Handled {} protocol stream from: {}",
                               self->ping_proto_->getProtocolId(),
                               peer_id.value().toHex());
              self->ping_proto_->handle(std::forward<decltype(stream)>(stream));
            }
          }
        });

    setProtocolHandler(fmt::format(kPropagateTransactionsProtocol.data(),
                                   chain_spec_->protocolId()),
                       &RouterLibp2p::handleTransactionsProtocol);

    block_announce_protocol_->start();
//    setProtocolHandler(
//        fmt::format(kBlockAnnouncesProtocol.data(), chain_spec_->protocolId()),
//        &RouterLibp2p::handleBlockAnnouncesProtocol);

    setProtocolHandler(
        fmt::format(kSyncProtocol.data(), chain_spec_->protocolId()),
        &RouterLibp2p::handleSyncProtocol);

    setProtocolHandler(kSupProtocol, &RouterLibp2p::handleSupProtocol);

    setProtocolHandler(kGossipProtocol, &RouterLibp2p::handleGossipProtocol);

    return true;
  }

  bool RouterLibp2p::start() {
    if (auto stream = loopback_stream_.lock()) {
      readAsyncMsg<GossipMessage>(
          std::move(stream),
          [](auto self, const auto &peer_id, const auto &msg) {
            return self->processGossipMessage(peer_id, msg);
          });
    }

    // IPv6
    {
      auto ma_res = libp2p::multi::Multiaddress::create(
          "/ip6/::/tcp/" + std::to_string(app_config_.p2pPort()) + "/p2p/"
          + own_info_.id.toBase58());
      BOOST_ASSERT(ma_res.has_value());
      auto &ma = ma_res.value();
      auto res = host_.listen(ma);
      if (not res) {
        log_->error("Cannot listen address {}. Error: {}",
                    ma.getStringAddress(),
                    res.error().message());
      }
    }

    // IPv4
    {
      auto ma_res = libp2p::multi::Multiaddress::create(
          "/ip4/0.0.0.0/tcp/" + std::to_string(app_config_.p2pPort()) + "/p2p/"
          + own_info_.id.toBase58());
      BOOST_ASSERT(ma_res.has_value());
      auto &ma = ma_res.value();
      auto res = host_.listen(ma);
      if (not res) {
        log_->error("Cannot listen address {}. Error: {}",
                    ma.getStringAddress(),
                    res.error().message());
      }
    }

    auto &addr_repo = host_.getPeerRepository().getAddressRepository();
    auto upsert_res = addr_repo.upsertAddresses(
        own_info_.id, own_info_.addresses, libp2p::peer::ttl::kPermanent);
    if (!upsert_res) {
      log_->error("Cannot add own addresses to repo: {}",
                  upsert_res.error().message());
    }

    host_.start();

    const auto &host_addresses = host_.getAddresses();
    if (host_addresses.empty()) {
      log_->critical("Host addresses is empty");
      return false;
    }

    log_->info("Started with peer id: {}", host_.getId().toBase58());
    for (auto &addr : host_addresses) {
      log_->info("Started listening on address: {}", addr.getStringAddress());
    }

    return true;
  }

  void RouterLibp2p::stop() {
    if (host_.getNetwork().getListener().isStarted()) {
      host_.stop();
    }
  }

  void RouterLibp2p::setProtocolHandler(
      const libp2p::peer::Protocol &protocol,
      void (RouterLibp2p::*method)(std::shared_ptr<Stream>) const) {
    host_.setProtocolHandler(
        protocol, [wp = weak_from_this(), protocol, method](auto &&stream) {
          if (auto self = wp.lock()) {
            if (auto peer_id = stream->remotePeerId()) {
              self->log_->trace("Handled {} protocol stream from: {}",
                                protocol,
                                peer_id.value().toHex());
              (self.get()->*method)(std::forward<decltype(stream)>(stream));
            } else {
              self->log_->warn("Handled {} protocol stream from unknown peer",
                               protocol);
            }
          }
        });
  }

  void RouterLibp2p::handleSyncProtocol(std::shared_ptr<Stream> stream) const {
    RPC<ProtobufMessageReadWriter>::read<BlocksRequest, BlocksResponse>(
        stream,
        [self{shared_from_this()}, stream](auto &&request) {
          // std::bind didn't work :(
          std::string from = visit_in_place(
              request.from,
              [](primitives::BlockNumber number) {
                return std::to_string(number);
              },
              [](const primitives::BlockHash &hash) { return hash.toHex(); });
          self->log_->debug(
              "Received request from peer {} requesting blocks from {} to {}",
              stream->remotePeerId().value().toBase58(),
              from,
              request.to->toHex());
          return self->sync_observer_->onBlocksRequest(
              std::forward<decltype(request)>(request));
        },
        [self{shared_from_this()}, stream](auto &&err) {
          self->log_->error(
              "error happened while processing message over Sync protocol: {}",
              err.error().message());
          stream->reset();
        });
  }

  void RouterLibp2p::handleBlockAnnouncesProtocol(
      std::shared_ptr<Stream> stream) const {
    Status status_msg;

    /// Roles
    // TODO(xDimon): Need to set actual role of node
    //  issue: https://github.com/soramitsu/kagome/issues/678
    status_msg.roles.flags.full = 1;

    /// Best block info
    const auto &last_finalized = block_tree_->getLastFinalized().block_hash;
    if (auto best_res =
            block_tree_->getBestContaining(last_finalized, boost::none);
        best_res.has_value()) {
      status_msg.best_block = best_res.value();
    } else {
      log_->error("Could not get best block info: {}",
                  best_res.error().message());
      return;
    }

    /// Genesis hash
    if (auto genesis_res = storage_->getGenesisBlockHash();
        genesis_res.has_value()) {
      status_msg.genesis_hash = std::move(genesis_res.value());
    } else {
      log_->error("Could not get genesis block hash: {}",
                  genesis_res.error().message());
      return;
    }

    readAsyncMsgWithHandshake<BlockAnnounce>(
        std::move(stream),
        std::move(status_msg),
        [](auto self,
           const auto &peer_id,
           const auto &remote_status) -> outcome::result<void> {
          BOOST_ASSERT(self);
          self->log_->debug("Received status from peer_id={}",
                            peer_id.toBase58());
          self->peer_manager_->updatePeerStatus(peer_id, remote_status);
          return outcome::success();
        },
        [](auto self, const auto &peer_id, const auto &block_announce) {
          BOOST_ASSERT(self);
          self->log_->info("Received block announce: block number {}",
                           block_announce.header.number);
          self->babe_observer_->onBlockAnnounce(peer_id, block_announce);

          auto hash = self->hasher_->blake2b_256(
              scale::encode(block_announce.header).value());
          self->peer_manager_->updatePeerStatus(
              peer_id, BlockInfo(block_announce.header.number, hash));
          return true;
        });
  }  // namespace kagome::network

  void RouterLibp2p::handleTransactionsProtocol(
      std::shared_ptr<Stream> stream) const {
    readAsyncMsgWithHandshake<PropagatedExtrinsics>(
        std::move(stream),
        NoData{},
        [](auto self, const auto &peer_id, const auto &msg)
            -> outcome::result<void> { return outcome::success(); },
        [](auto self, const auto &peer_id, const auto &msg) {
          BOOST_ASSERT(self);
          self->log_->info("Received {} propagated transactions",
                           msg.extrinsics.size());
          for (auto &ext : msg.extrinsics) {
            auto result = self->extrinsic_observer_->onTxMessage(ext);
            if (result) {
              self->log_->debug("  Received tx {}", result.value());
            } else {
              self->log_->debug("  Rejected tx: {}", result.error().message());
            }
          }
          return true;
        });
  }

  void RouterLibp2p::handleGossipProtocol(
      std::shared_ptr<Stream> stream) const {
    if (stream_engine_->add(stream, kGossipProtocol))
      readAsyncMsg<GossipMessage>(
          std::move(stream),
          [](auto self, const auto &peer_id, const auto &msg) {
            return self->processGossipMessage(peer_id, msg);
          });
  }

  void RouterLibp2p::handleSupProtocol(std::shared_ptr<Stream> stream) const {
    Status status_msg;
    status_msg.best_block = {};
    status_msg.roles.flags.full = 1;

    {  /// Genesis hash
      auto genesis_res = storage_->getGenesisBlockHash();
      if (genesis_res) {
        status_msg.genesis_hash = std::move(genesis_res.value());
      } else {
        log_->error("Could not get genesis hash: {}",
                    genesis_res.error().message());
        return;
      }
    }
    {  /// Best hash
      auto best_res = storage_->getLastFinalizedBlockHash();
      if (best_res) {
        status_msg.best_block.block_hash = std::move(best_res.value());
      } else {
        log_->error("Could not get best hash: {}", best_res.error().message());
        return;
      }
    }

    auto rw = std::make_shared<ScaleMessageReadWriter>(std::move(stream));
    rw->write(status_msg, [self{shared_from_this()}](auto &&res) {
      if (!res)
        self->log_->error("Could not broadcast, reason: {}",
                          res.error().message());
    });

    if (stream_engine_->add(stream, kSupProtocol)) {
      readAsyncMsg<Status>(std::move(stream),
                           [](auto self, const auto &peer_id, const auto &msg) {
                             return self->processSupMessage(peer_id, msg);
                           });
    }
  }

  bool RouterLibp2p::processGossipMessage(const libp2p::peer::PeerId &peer_id,
                                          const GossipMessage &msg) const {
    using MsgType = GossipMessage::Type;

    switch (msg.type) {
      case MsgType::BLOCK_ANNOUNCE: {
        BOOST_ASSERT(!"Legacy protocol unsupported!");
        log_->warn("Legacy protocol message BLOCK_ANNOUNCE from: {}",
                   peer_id.toBase58());
        return false;
      }
      case GossipMessage::Type::CONSENSUS: {
        auto grandpa_msg_res = scale::decode<GrandpaMessage>(msg.data);

        if (not grandpa_msg_res) {
          log_->error("error while decoding a consensus (grandpa) message: {}",
                      grandpa_msg_res.error().message());
          return false;
        }

        auto &grandpa_msg = grandpa_msg_res.value();

        return visit_in_place(
            grandpa_msg,
            [this, &peer_id](const network::GrandpaVoteMessage &vote_message) {
              grandpa_observer_->onVoteMessage(peer_id, vote_message);
              return true;
            },
            [this, &peer_id](const network::GrandpaPreCommit &fin_message) {
              grandpa_observer_->onFinalize(peer_id, fin_message);
              return true;
            },
            [](const GrandpaNeighborPacket &neighbor_packet) {
              BOOST_ASSERT_MSG(false,
                               "Unimplemented variant (GrandpaNeighborPacket) "
                               "of consensus (grandpa) message");
              return false;
            },
            [this, &peer_id](const network::CatchUpRequest &catch_up_request) {
              grandpa_observer_->onCatchUpRequest(peer_id, catch_up_request);
              return true;
            },
            [this,
             &peer_id](const network::CatchUpResponse &catch_up_response) {
              grandpa_observer_->onCatchUpResponse(peer_id, catch_up_response);
              return true;
            },
            [](const auto &...) {
              BOOST_ASSERT_MSG(
                  false, "Unknown variant of consensus (grandpa) message");
              return false;
            });
      }
      case MsgType::TRANSACTIONS: {
        BOOST_ASSERT(!"Legacy protocol unsupported!");
        log_->warn("Legacy protocol message TRANSACTIONS from: {}",
                   peer_id.toBase58());
        return false;
      }
      case GossipMessage::Type::STATUS: {
        log_->error("Status message processing is not implemented yet");
        return false;
      }
      case GossipMessage::Type::BLOCK_REQUEST: {
        log_->error("BlockRequest message processing is not implemented yet");
        return false;
      }
      case GossipMessage::Type::UNKNOWN:
      default: {
        log_->error("unknown message type is set");
        return false;
      }
    }
  }

  bool RouterLibp2p::processSupMessage(const libp2p::peer::PeerId &peer_id,
                                       const Status &status) const {
    peer_manager_->updatePeerStatus(peer_id, status);
    return true;
  }
}  // namespace kagome::network
