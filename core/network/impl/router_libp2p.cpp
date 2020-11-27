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
#include "network/types/no_data_message.hpp"
#include "network/types/peer_list.hpp"
#include "network/types/status.hpp"
#include "scale/scale.hpp"

namespace kagome::network {
  RouterLibp2p::RouterLibp2p(
      libp2p::Host &host,
      std::shared_ptr<BabeObserver> babe_observer,
      std::shared_ptr<consensus::grandpa::GrandpaObserver> grandpa_observer,
      std::shared_ptr<SyncProtocolObserver> sync_observer,
      std::shared_ptr<ExtrinsicObserver> extrinsic_observer,
      std::shared_ptr<Gossiper> gossiper,
      const PeerList &peer_list,
      const OwnPeerInfo &own_peer_info,
      std::shared_ptr<kagome::application::ChainSpec> config,
      std::shared_ptr<blockchain::BlockStorage> storage,
      std::shared_ptr<libp2p::protocol::Identify> identify,
      std::shared_ptr<libp2p::protocol::Ping> ping_proto)
      : host_{host},
        babe_observer_{std::move(babe_observer)},
        grandpa_observer_{std::move(grandpa_observer)},
        sync_observer_{std::move(sync_observer)},
        extrinsic_observer_{std::move(extrinsic_observer)},
        gossiper_{std::move(gossiper)},
        log_{common::createLogger("RouterLibp2p")},
        config_(std::move(config)),
        transactions_protocol_{fmt::format(
            kPropagateTransactionsProtocol.data(), config_->protocolId())},
        block_announces_protocol_{
            fmt::format(kBlockAnnouncesProtocol.data(), config_->protocolId())},
        storage_{std::move(storage)},
        identify_{std::move(identify)},
        ping_proto_{std::move(ping_proto)} {
    BOOST_ASSERT_MSG(babe_observer_ != nullptr, "babe observer is nullptr");
    BOOST_ASSERT_MSG(grandpa_observer_ != nullptr,
                     "grandpa observer is nullptr");
    BOOST_ASSERT_MSG(sync_observer_ != nullptr, "sync observer is nullptr");
    BOOST_ASSERT_MSG(extrinsic_observer_ != nullptr,
                     "author api observer is nullptr");
    BOOST_ASSERT_MSG(gossiper_ != nullptr, "gossiper is nullptr");
    BOOST_ASSERT_MSG(!peer_list.peers.empty(), "peer list is empty");
    BOOST_ASSERT(storage_ != nullptr);
    BOOST_ASSERT(identify_ != nullptr);
    BOOST_ASSERT(ping_proto_ != nullptr);

    gossiper_->storeSelfPeerInfo(own_peer_info);
    for (const auto &peer_info : peer_list.peers) {
      if (peer_info.id != own_peer_info.id) {
        gossiper_->reserveStream(peer_info, transactions_protocol_, {});
        gossiper_->reserveStream(peer_info, block_announces_protocol_, {});
        gossiper_->reserveStream(peer_info, kGossipProtocol, {});
      } else {
        auto stream = std::make_shared<LoopbackStream>(own_peer_info);
        loopback_stream_ = stream;
        gossiper_->reserveStream(
            own_peer_info, kGossipProtocol, std::move(stream));
      }
    }
  }

  void RouterLibp2p::init() {
    host_.setProtocolHandler(
        fmt::format(kSyncProtocol.data(), config_->protocolId()),
        [self{shared_from_this()}](auto &&stream) {
          self->handleSyncProtocol(std::forward<decltype(stream)>(stream));
        });
    host_.setProtocolHandler(transactions_protocol_,
                             [self{shared_from_this()}](auto &&stream) {
                               self->handleTransactionsProtocol(
                                   std::forward<decltype(stream)>(stream));
                             });
    host_.setProtocolHandler(block_announces_protocol_,
                             [self{shared_from_this()}](auto &&stream) {
                               self->handleBlockAnnouncesProtocol(
                                   std::forward<decltype(stream)>(stream));
                             });
    host_.setProtocolHandler(identify_->getProtocolId(),
                             [wself{weak_from_this()}](auto &&stream) {
                               if (auto self = wself.lock())
                                 self->identify_->handle(stream);
                             });
    host_.setProtocolHandler(ping_proto_->getProtocolId(),
                             [wself{weak_from_this()}](auto &&stream) {
                               if (auto self = wself.lock())
                                 self->ping_proto_->handle(stream);
                             });
    host_.setProtocolHandler(
        network::kSupProtocol, [wself{weak_from_this()}](auto &&stream) {
          if (auto self = wself.lock(); self && stream)
            if (auto peer_id = stream->remotePeerId())
              self->log_->info("Handled {} protocol stream from: {}",
                               network::kSupProtocol,
                               peer_id.value().toHex());
        });
    new_connection_handler_ = host_.setOnNewConnectionHandler(
        [wself{weak_from_this()}](auto &&peer_info) {
          if (auto self = wself.lock()) {
            self->host_.newStream(
                peer_info,
                self->block_announces_protocol_,
                [self, peer_info](auto &&stream_res) {
                  if (!stream_res) {
                    self->log_->warn("Unable to create stream with {}",
                                     peer_info.id.toHex());
                    return;
                  }
                  self->gossiper_->addStream(self->block_announces_protocol_,
                                             stream_res.value());
                });
          }
        });
    host_.setProtocolHandler(
        kGossipProtocol, [self{shared_from_this()}](auto &&stream) {
          self->handleGossipProtocol(std::forward<decltype(stream)>(stream));
        });
    if (auto stream = loopback_stream_.lock()) {
      readAsyncMsg<GossipMessage>(
          std::move(stream),
          [](auto self, const auto &peer_id, const auto &msg) {
            return self->processGossipMessage(peer_id, msg);
          });
    }
    host_.start();
    const auto &host_addresses = host_.getAddresses();
    BOOST_ASSERT_MSG(not host_addresses.empty(), "Host addresses empty");
    log_->info("Started listening with peer id: {} on address: {}",
               host_.getId().toBase58(),
               host_addresses.front().getStringAddress());
  }

  void RouterLibp2p::handleSyncProtocol(
      const std::shared_ptr<Stream> &stream) const {
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
              "Received request from peer {} requesting blocks from {}"
              " to {}",
              stream->remotePeerId().value().toBase58(),
              from,
              request.to->toHex());
          return self->sync_observer_->onBlocksRequest(
              std::forward<decltype(request)>(request));
        },
        [self{shared_from_this()}, stream](auto &&err) {
          self->log_->error(
              "error happened while processing request/response over Sync "
              "protocol: {}",
              err.error().message());
          stream->reset();
        });
  }

  void RouterLibp2p::handleBlockAnnouncesProtocol(
      std::shared_ptr<Stream> stream) const {
    Status status_msg;

    /// TODO(iceseer): #589 use last finalized and best number
    status_msg.best_number = 0;
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
      /// TODO(iceseer): #589 use last finalized and best number
      auto best_res =
          storage_
              ->getGenesisBlockHash();  // storage_->getLastFinalizedBlockHash();
      if (best_res) {
        status_msg.best_hash = std::move(best_res.value());
      } else {
        log_->error("Could not get best hash: {}", best_res.error().message());
        return;
      }
    }

    readAsyncMsgWithHandshake<BlockAnnounce>(
        std::move(stream),
        std::move(status_msg),
        [](auto self, const auto &peer_id, const auto &msg) {
          BOOST_ASSERT(self);
          self->log_->info("Received block announce: block number {}",
                           msg.header.number);
          self->babe_observer_->onBlockAnnounce(peer_id, msg);
          return true;
        });
  }

  void RouterLibp2p::handleTransactionsProtocol(
      std::shared_ptr<Stream> stream) const {
    readAsyncMsgWithHandshake<PropagatedTransactions>(
        std::move(stream),
        NoData{},
        [](auto self, const auto &peer_id, const auto &msg) {
          BOOST_ASSERT(self);
          self->log_->info("Received propagated transactions: {} txs",
                           msg.extrinsics.size());
          for (auto &extrinsic : msg.extrinsics) {
            auto result = self->extrinsic_observer_->onTxMessage(extrinsic);
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
    if (gossiper_->addStream(kGossipProtocol, stream))
      readAsyncMsg<GossipMessage>(
          std::move(stream),
          [](auto self, const auto &peer_id, const auto &msg) {
            return self->processGossipMessage(peer_id, msg);
          });
  }

  void RouterLibp2p::handleSupProtocol(std::shared_ptr<Stream> stream) const {
    Status status_msg;
    status_msg.best_number = 0;
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
        status_msg.best_hash = std::move(best_res.value());
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
  }

  bool RouterLibp2p::processGossipMessage(const libp2p::peer::PeerId &peer_id,
                                          const GossipMessage &msg) const {
    using MsgType = GossipMessage::Type;

    switch (msg.type) {
      case MsgType::BLOCK_ANNOUNCE: {
        BOOST_ASSERT(!"Legacy protocol unsupported!");
        log_->warn("Legacy protocol message BLOCK_ANNOUNCE from: {}",
                   peer_id.toHex());
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
                   peer_id.toHex());
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
}  // namespace kagome::network
