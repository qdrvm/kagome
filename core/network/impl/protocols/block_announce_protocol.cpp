/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/protocols/block_announce_protocol.hpp"

#include "application/app_configuration.hpp"
#include "network/common.hpp"
#include "network/helpers/peer_id_formatter.hpp"
#include "network/helpers/scale_message_read_writer.hpp"
#include "network/impl/protocols/protocol_error.hpp"

namespace kagome::network {

  KAGOME_DEFINE_CACHE(BlockAnnounceProtocol);

  BlockAnnounceProtocol::BlockAnnounceProtocol(
      libp2p::Host &host,
      const application::AppConfiguration &app_config,
      const application::ChainSpec &chain_spec,
      std::shared_ptr<StreamEngine> stream_engine,
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<BlockAnnounceObserver> observer,
      std::shared_ptr<PeerManager> peer_manager)
      : base_(host,
              {fmt::format(kBlockAnnouncesProtocol.data(),
                           chain_spec.protocolId())},
              "BlockAnnounceProtocol"),
        app_config_(app_config),
        stream_engine_(std::move(stream_engine)),
        block_tree_(std::move(block_tree)),
        observer_(std::move(observer)),
        peer_manager_(std::move(peer_manager)) {
    BOOST_ASSERT(stream_engine_ != nullptr);
    BOOST_ASSERT(block_tree_ != nullptr);
    BOOST_ASSERT(observer_ != nullptr);
    BOOST_ASSERT(peer_manager_ != nullptr);
  }

  bool BlockAnnounceProtocol::start() {
    return base_.start(weak_from_this());
  }

  bool BlockAnnounceProtocol::stop() {
    return base_.stop();
  }

  outcome::result<Status> BlockAnnounceProtocol::createStatus() const {
    /// Roles
    Roles roles = app_config_.roles();

    /// Best block info
    BlockInfo best_block;
    const auto &last_finalized = block_tree_->getLastFinalized().hash;
    if (auto best_res =
            block_tree_->getBestContaining(last_finalized, std::nullopt);
        best_res.has_value()) {
      best_block = best_res.value();
    } else {
      base_.logger()->error("Could not get best block info: {}",
                            best_res.error());
      return ProtocolError::CAN_NOT_CREATE_STATUS;
    }

    auto &genesis_hash = block_tree_->getGenesisBlockHash();

    return Status{
        .roles = roles, .best_block = best_block, .genesis_hash = genesis_hash};
  }

  void BlockAnnounceProtocol::onIncomingStream(std::shared_ptr<Stream> stream) {
    BOOST_ASSERT(stream->remotePeerId().has_value());

    readStatus(stream,
               Direction::INCOMING,
               [wp = weak_from_this(), stream](outcome::result<void> res) {
                 auto self = wp.lock();
                 if (not self) {
                   stream->reset();
                   return;
                 }

                 auto peer_id = stream->remotePeerId().value();

                 if (not res.has_value()) {
                   SL_VERBOSE(
                       self->base_.logger(),
                       "Handshake failed on incoming {} stream with {}: {}",
                       self->protocolName(),
                       peer_id.toBase58(),
                       res.error());
                   stream->reset();
                   return;
                 }

                 res = self->stream_engine_->addIncoming(stream, self);
                 if (not res.has_value()) {
                   SL_VERBOSE(self->base_.logger(),
                              "Can't register incoming {} stream with {}: {}",
                              self->protocolName(),
                              peer_id.toBase58(),
                              res.error());
                   stream->reset();
                   return;
                 }

                 self->peer_manager_->reserveStreams(peer_id);
                 self->peer_manager_->startPingingPeer(peer_id);

                 SL_VERBOSE(self->base_.logger(),
                            "Fully established incoming {} stream with {}",
                            self->protocolName(),
                            peer_id.toBase58());
               });
  }

  void BlockAnnounceProtocol::newOutgoingStream(
      const PeerInfo &peer_info,
      std::function<void(outcome::result<std::shared_ptr<Stream>>)> &&cb) {
    SL_DEBUG(base_.logger(),
             "Connect for {} stream with {}",
             protocolName(),
             peer_info.id);

    base_.host().newStream(
        peer_info.id,
        base_.protocolIds(),
        [wp = weak_from_this(), peer_id = peer_info.id, cb = std::move(cb)](
            auto &&stream_res) mutable {
          auto self = wp.lock();
          if (not self) {
            cb(ProtocolError::GONE);
            return;
          }

          if (not stream_res.has_value()) {
            SL_VERBOSE(self->base_.logger(),
                       "Can't create outgoing {} stream with {}: {}",
                       self->protocolName(),
                       peer_id,
                       stream_res.error());
            cb(stream_res.as_failure());
            return;
          }

          const auto &stream_and_proto = stream_res.value();

          auto cb2 = [wp,
                      stream = stream_and_proto.stream,
                      protocol = stream_and_proto.protocol,
                      cb = std::move(cb)](outcome::result<void> res) {
            auto self = wp.lock();
            if (not self) {
              cb(ProtocolError::GONE);
              return;
            }

            if (not res.has_value()) {
              SL_VERBOSE(self->base_.logger(),
                         "Handshake failed on outgoing {} stream with {}: {}",
                         protocol,
                         stream->remotePeerId().value(),
                         res.error());
              stream->reset();
              cb(res.as_failure());
              return;
            }

            res = self->stream_engine_->addOutgoing(stream, self);
            if (not res.has_value()) {
              SL_VERBOSE(self->base_.logger(),
                         "Can't register outgoing {} stream with {}: {}",
                         protocol,
                         stream->remotePeerId().value(),
                         res.error());
              stream->reset();
              cb(res.as_failure());
              return;
            }

            SL_VERBOSE(self->base_.logger(),
                       "Fully established outgoing {} stream with {}",
                       protocol,
                       stream->remotePeerId().value());
            cb(std::move(stream));
          };

          self->writeStatus(std::move(stream_and_proto.stream),
                            Direction::OUTGOING,
                            std::move(cb2));
        });
  }

  void BlockAnnounceProtocol::readStatus(
      std::shared_ptr<Stream> stream,
      Direction direction,
      std::function<void(outcome::result<void>)> &&cb) {
    auto read_writer = std::make_shared<ScaleMessageReadWriter>(stream);

    read_writer->read<Status>(
        [stream, direction, wp = weak_from_this(), cb = std::move(cb)](
            auto &&remote_status_res) mutable {
          auto self = wp.lock();
          if (not self) {
            stream->reset();
            cb(ProtocolError::GONE);
            return;
          }

          if (not remote_status_res.has_value()) {
            SL_VERBOSE(self->base_.logger(),
                       "Can't read handshake from {}: {}",
                       stream->remotePeerId().value(),
                       remote_status_res.error());
            stream->reset();
            cb(remote_status_res.as_failure());
            return;
          }
          auto &remote_status = remote_status_res.value();

          SL_TRACE(self->base_.logger(),
                   "Handshake has received from {}",
                   stream->remotePeerId().value());

          auto &genesis_hash = self->block_tree_->getGenesisBlockHash();

          if (remote_status.genesis_hash != genesis_hash) {
            SL_VERBOSE(self->base_.logger(),
                       "Error while processing status: genesis no match");
            stream->reset();
            cb(ProtocolError::GENESIS_NO_MATCH);
            return;
          }

          auto peer_id = stream->remotePeerId().value();
          SL_TRACE(self->base_.logger(),
                   "Received status from peer_id={} (best block {})",
                   peer_id,
                   remote_status.best_block.number);
          self->peer_manager_->updatePeerState(peer_id, remote_status);

          switch (direction) {
            case Direction::OUTGOING:
              cb(outcome::success());
              break;
            case Direction::INCOMING:
              self->writeStatus(
                  std::move(stream), Direction::INCOMING, std::move(cb));
              break;
          }

          self->observer_->onRemoteStatus(peer_id, remote_status);
        });
  }

  void BlockAnnounceProtocol::writeStatus(
      std::shared_ptr<Stream> stream,
      Direction direction,
      std::function<void(outcome::result<void>)> &&cb) {
    auto read_writer = std::make_shared<ScaleMessageReadWriter>(stream);

    auto status_res = createStatus();
    if (not status_res.has_value()) {
      stream->reset();
      cb(ProtocolError::CAN_NOT_CREATE_STATUS);
      return;
    }

    const auto &status = status_res.value();

    read_writer->write(status,
                       [stream = std::move(stream),
                        direction,
                        wp = weak_from_this(),
                        cb = std::move(cb)](auto &&write_res) mutable {
                         auto self = wp.lock();
                         if (not self) {
                           stream->reset();
                           cb(ProtocolError::GONE);
                           return;
                         }

                         if (not write_res.has_value()) {
                           SL_VERBOSE(self->base_.logger(),
                                      "Can't send handshake to {}: {}",
                                      stream->remotePeerId().value(),
                                      write_res.error());
                           stream->reset();
                           cb(write_res.as_failure());
                           return;
                         }

                         SL_TRACE(self->base_.logger(),
                                  "Handshake has sent to {}",
                                  stream->remotePeerId().value());

                         switch (direction) {
                           case Direction::OUTGOING:
                             self->readStatus(std::move(stream),
                                              Direction::OUTGOING,
                                              std::move(cb));
                             break;
                           case Direction::INCOMING:
                             cb(outcome::success());
                             self->readAnnounce(std::move(stream));
                             break;
                         }
                       });
  }

  void BlockAnnounceProtocol::readAnnounce(std::shared_ptr<Stream> stream) {
    auto read_writer = std::make_shared<ScaleMessageReadWriter>(stream);

    read_writer->read<BlockAnnounce>(
        [stream = std::move(stream),
         wp = weak_from_this()](auto &&block_announce_res) mutable {
          auto self = wp.lock();
          if (not self) {
            stream->reset();
            return;
          }

          if (not block_announce_res.has_value()) {
            SL_DEBUG(self->base_.logger(),
                     "Can't read block announce from {}: {}",
                     stream->remotePeerId().value(),
                     block_announce_res.error());
            stream->reset();
            return;
          }

          auto peer_id = stream->remotePeerId().value();
          auto &block_announce = block_announce_res.value();

          SL_VERBOSE(self->base_.logger(),
                     "Announce of block #{} is received from {}",
                     block_announce.header.number,
                     peer_id);

          self->observer_->onBlockAnnounce(peer_id, block_announce);

          BOOST_ASSERT_MSG(stream->remotePeerId().has_value(),
                           "peer_id must be known at this moment");
          self->peer_manager_->updatePeerState(stream->remotePeerId().value(),
                                               block_announce);

          self->readAnnounce(std::move(stream));
        });
  }

  void BlockAnnounceProtocol::blockAnnounce(BlockAnnounce &&announce) {
    auto shared_msg =
        KAGOME_EXTRACT_SHARED_CACHE(BlockAnnounceProtocol, BlockAnnounce);
    (*shared_msg) = std::move(announce);

    SL_DEBUG(
        base_.logger(), "Send announce of block #{}", announce.header.number);

    stream_engine_->broadcast(
        shared_from_this(),
        shared_msg,
        StreamEngine::RandomGossipStrategy{
            stream_engine_->outgoingStreamsNumber(shared_from_this()),
            app_config_.luckyPeers()});
  }

}  // namespace kagome::network
