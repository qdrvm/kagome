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
      std::shared_ptr<blockchain::BlockStorage> storage,
      std::shared_ptr<BlockAnnounceObserver> observer,
      std::shared_ptr<PeerManager> peer_manager)
      : host_(host),
        app_config_(app_config),
        stream_engine_(std::move(stream_engine)),
        block_tree_(std::move(block_tree)),
        storage_(std::move(storage)),
        observer_(std::move(observer)),
        peer_manager_(std::move(peer_manager)) {
    BOOST_ASSERT(stream_engine_ != nullptr);
    BOOST_ASSERT(block_tree_ != nullptr);
    BOOST_ASSERT(storage_ != nullptr);
    BOOST_ASSERT(observer_ != nullptr);
    BOOST_ASSERT(peer_manager_ != nullptr);
    const_cast<Protocol &>(protocol_) =
        fmt::format(kBlockAnnouncesProtocol.data(), chain_spec.protocolId());
  }

  bool BlockAnnounceProtocol::start() {
    host_.setProtocolHandler(protocol_, [wp = weak_from_this()](auto &&stream) {
      if (auto self = wp.lock()) {
        if (auto peer_id = stream->remotePeerId()) {
          SL_TRACE(self->log_,
                   "Handled {} protocol stream from: {}",
                   self->protocol_,
                   peer_id.value().toBase58());
          self->onIncomingStream(std::forward<decltype(stream)>(stream));
          return;
        }
        self->log_->warn("Handled {} protocol stream from unknown peer",
                         self->protocol_);
        stream->reset();
      }
    });
    return true;
  }

  bool BlockAnnounceProtocol::stop() {
    return true;
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
      log_->error("Could not get best block info: {}",
                  best_res.error().message());
      return ProtocolError::CAN_NOT_CREATE_STATUS;
    }

    /// Genesis hash
    BlockHash genesis_hash;
    if (auto genesis_res = storage_->getGenesisBlockHash();
        genesis_res.has_value()) {
      genesis_hash = std::move(genesis_res.value());
    } else {
      log_->error("Could not get genesis block hash: {}",
                  genesis_res.error().message());
      return ProtocolError::CAN_NOT_CREATE_STATUS;
    }

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
                       self->log_,
                       "Handshake failed on incoming {} stream with {}: {}",
                       self->protocol_,
                       peer_id.toBase58(),
                       res.error().message());
                   stream->reset();
                   return;
                 }

                 res = self->stream_engine_->addIncoming(stream, self);
                 if (not res.has_value()) {
                   SL_VERBOSE(self->log_,
                              "Can't register incoming {} stream with {}: {}",
                              self->protocol_,
                              peer_id.toBase58(),
                              res.error().message());
                   stream->reset();
                   return;
                 }

                 self->peer_manager_->reserveStreams(peer_id);
                 self->peer_manager_->startPingingPeer(peer_id);

                 SL_VERBOSE(self->log_,
                            "Fully established incoming {} stream with {}",
                            self->protocol_,
                            peer_id.toBase58());
               });
  }

  void BlockAnnounceProtocol::newOutgoingStream(
      const PeerInfo &peer_info,
      std::function<void(outcome::result<std::shared_ptr<Stream>>)> &&cb) {
    SL_DEBUG(log_, "Connect for {} stream with {}", protocol_, peer_info.id);

    host_.newStream(
        peer_info.id,
        protocol_,
        [wp = weak_from_this(), peer_id = peer_info.id, cb = std::move(cb)](
            auto &&stream_res) mutable {
          auto self = wp.lock();
          if (not self) {
            cb(ProtocolError::GONE);
            return;
          }

          if (not stream_res.has_value()) {
            SL_VERBOSE(self->log_,
                       "Can't create outgoing {} stream with {}: {}",
                       self->protocol_,
                       peer_id,
                       stream_res.error().message());
            cb(stream_res.as_failure());
            return;
          }

          auto &stream = stream_res.value();

          auto cb2 = [wp, stream, cb = std::move(cb)](
                         outcome::result<void> res) {
            auto self = wp.lock();
            if (not self) {
              cb(ProtocolError::GONE);
              return;
            }

            if (not res.has_value()) {
              SL_VERBOSE(self->log_,
                         "Handshake failed on outgoing {} stream with {}: {}",
                         self->protocol_,
                         stream->remotePeerId().value(),
                         res.error().message());
              stream->reset();
              cb(res.as_failure());
              return;
            }

            res = self->stream_engine_->addOutgoing(stream, self);
            if (not res.has_value()) {
              SL_VERBOSE(self->log_,
                         "Can't register outgoing {} stream with {}: {}",
                         self->protocol_,
                         stream->remotePeerId().value(),
                         res.error().message());
              stream->reset();
              cb(res.as_failure());
              return;
            }

            SL_VERBOSE(self->log_,
                       "Fully established outgoing {} stream with {}",
                       self->protocol_,
                       stream->remotePeerId().value());
            cb(std::move(stream));
          };

          self->writeStatus(
              std::move(stream), Direction::OUTGOING, std::move(cb2));
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
            SL_VERBOSE(self->log_,
                       "Can't read handshake from {}: {}",
                       stream->remotePeerId().value(),
                       remote_status_res.error().message());
            stream->reset();
            cb(remote_status_res.as_failure());
            return;
          }
          auto &remote_status = remote_status_res.value();

          SL_TRACE(self->log_,
                   "Handshake has received from {}",
                   stream->remotePeerId().value());

          if (auto genesis_res = self->storage_->getGenesisBlockHash();
              genesis_res.has_value()) {
            if (remote_status.genesis_hash != genesis_res.value()) {
              SL_VERBOSE(self->log_,
                         "Error while processing status: genesis no match");
              stream->reset();
              cb(ProtocolError::GENESIS_NO_MATCH);
              return;
            }
          } else {
            SL_VERBOSE(self->log_,
                       "Error while processing status: {}",
                       genesis_res.error().message());
            stream->reset();
            cb(ProtocolError::GENESIS_NO_MATCH);
            return;
          }

          auto peer_id = stream->remotePeerId().value();
          SL_TRACE(self->log_,
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

    read_writer->write(status_res.value(),
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
                           SL_VERBOSE(self->log_,
                                      "Can't send handshake to {}: {}",
                                      stream->remotePeerId().value(),
                                      write_res.error().message());
                           stream->reset();
                           cb(write_res.as_failure());
                           return;
                         }

                         SL_TRACE(self->log_,
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
            SL_WARN(self->log_,
                    "Can't read block announce from {}: {}",
                    stream->remotePeerId().value(),
                    block_announce_res.error().message());
            stream->reset();
            return;
          }

          auto peer_id = stream->remotePeerId().value();
          auto &block_announce = block_announce_res.value();

          SL_VERBOSE(self->log_,
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

    SL_DEBUG(log_, "Send announce of block #{}", announce.header.number);

    stream_engine_->broadcast<BlockAnnounce>(shared_from_this(),
                                             std::move(shared_msg));
  }

}  // namespace kagome::network
