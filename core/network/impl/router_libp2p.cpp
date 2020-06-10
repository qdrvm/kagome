/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/router_libp2p.hpp"

#include "consensus/grandpa/structs.hpp"
#include "network/common.hpp"
#include "network/rpc.hpp"
#include "network/types/block_announce.hpp"
#include "network/types/blocks_request.hpp"
#include "network/types/blocks_response.hpp"
#include "scale/scale.hpp"

namespace kagome::network {
  RouterLibp2p::RouterLibp2p(
      libp2p::Host &host,
      std::shared_ptr<BabeObserver> babe_observer,
      std::shared_ptr<consensus::grandpa::RoundObserver> grandpa_observer,
      std::shared_ptr<SyncProtocolObserver> sync_observer,
      std::shared_ptr<ExtrinsicObserver> extrinsic_observer,
      std::shared_ptr<Gossiper> gossiper)
      : host_{host},
        babe_observer_{std::move(babe_observer)},
        grandpa_observer_{std::move(grandpa_observer)},
        sync_observer_{std::move(sync_observer)},
        extrinsic_observer_{std::move(extrinsic_observer)},
        gossiper_{std::move(gossiper)},
        log_{common::createLogger("RouterLibp2p")} {
    BOOST_ASSERT_MSG(babe_observer_ != nullptr, "babe observer is nullptr");
    BOOST_ASSERT_MSG(grandpa_observer_ != nullptr,
                     "grandpa observer is nullptr");
    BOOST_ASSERT_MSG(sync_observer_ != nullptr, "sync observer is nullptr");
    BOOST_ASSERT_MSG(extrinsic_observer_ != nullptr,
                     "author api observer is nullptr");
    BOOST_ASSERT_MSG(gossiper_ != nullptr, "gossiper is nullptr");
  }

  void RouterLibp2p::init() {
    host_.setProtocolHandler(
        kSyncProtocol, [self{shared_from_this()}](auto &&stream) {
          self->handleSyncProtocol(std::forward<decltype(stream)>(stream));
        });
    host_.setProtocolHandler(
        kGossipProtocol, [self{shared_from_this()}](auto &&stream) {
          self->handleGossipProtocol(std::forward<decltype(stream)>(stream));
        });
    host_.start();
    const auto &host_addresses = host_.getAddresses();
    BOOST_ASSERT_MSG(not host_addresses.empty(), "Host addresses empty");
    log_->info("Started listening with peer id: {} on address: {}",
               host_.getId().toBase58(),
               host_addresses.front().getStringAddress());
  }

  void RouterLibp2p::handleSyncProtocol(
      const std::shared_ptr<Stream> &stream) const {
    RPC<ScaleMessageReadWriter>::read<BlocksRequest, BlocksResponse>(
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

  void RouterLibp2p::handleGossipProtocol(
      std::shared_ptr<Stream> stream) const {
    gossiper_->addStream(stream);
    return readGossipMessage(std::move(stream));
  }

  void RouterLibp2p::readGossipMessage(std::shared_ptr<Stream> stream) const {
    auto read_writer = std::make_shared<ScaleMessageReadWriter>(stream);
    read_writer->read<GossipMessage>(
        [self{shared_from_this()},
         stream = std::move(stream)](auto &&msg_res) mutable {
          if (!msg_res) {
            self->log_->error("error while reading gossip message: {}",
                              msg_res.error().message());
            return stream->reset();
          }

          if (!self->processGossipMessage(msg_res.value())) {
            stream->reset();
            return;
          }
          self->readGossipMessage(stream);
        });
  }

  bool RouterLibp2p::processGossipMessage(const GossipMessage &msg) const {
    using MsgType = GossipMessage::Type;

    switch (msg.type) {
      case MsgType::BLOCK_ANNOUNCE: {
        auto msg_res = scale::decode<BlockAnnounce>(msg.data);
        if (!msg_res) {
          log_->error("error while decoding a block announce message: {}",
                      msg_res.error().message());
          return false;
        }
        log_->info("Received block announce: block number {}",
                   msg_res.value().header.number);
        babe_observer_->onBlockAnnounce(msg_res.value());
        return true;
      }
      case MsgType::CONSENSUS: {
        auto vote_msg_res =
            scale::decode<consensus::grandpa::VoteMessage>(msg.data);
        if (vote_msg_res) {
          grandpa_observer_->onVoteMessage(vote_msg_res.value());
          return true;
        }

        auto fin_msg_res = scale::decode<consensus::grandpa::Fin>(msg.data);
        if (fin_msg_res) {
          grandpa_observer_->onFinalize(fin_msg_res.value());
          return true;
        }

        log_->error("error while decoding a consensus message");
        return false;
      }
      case MsgType::TRANSACTIONS: {
        auto txs_msg_res =
            scale::decode<std::vector<primitives::Extrinsic>>(msg.data);

        if (not txs_msg_res) {
          log_->error("error while decoding a transactions message");
          return false;
        }

        log_->info("Received tx announce: {} txs", txs_msg_res.value().size());

        for (auto &extrinsic : txs_msg_res.value()) {
          auto result = extrinsic_observer_->onTxMessage(extrinsic);
          if (result) {
            log_->debug("  Received tx {}", result.value());
          } else {
            log_->debug("  Rejected tx: {}", result.error().message());
          }
        }
        return true;
      }
      case GossipMessage::Type::STATUS: {
        log_->error("Status message processing is not implemented yet");
        return false;
      }
      case GossipMessage::Type::BLOCK_REQUEST: {
        log_->error("BlockRequest message processing is not implemented yet");
        return false;
      }
      case GossipMessage::Type::UNKNOWN: {
        log_->error("unknown message type is set");
        return false;
      }
    }
  }
}  // namespace kagome::network
