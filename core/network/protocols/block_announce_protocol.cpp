/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/protocols/block_announce_protocol.hpp"
#include <application/app_configuration.hpp>

#include "network/common.hpp"
#include "network/helpers/scale_message_read_writer.hpp"
#include "network/protocols/protocol_error.hpp"

namespace kagome::network {

  BlockAnnounceProtocol::BlockAnnounceProtocol(
      libp2p::Host &host,
      const application::AppConfiguration &app_config,
      const application::ChainSpec &chain_spec,
      std::shared_ptr<StreamEngine> stream_engine,
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<blockchain::BlockStorage> storage,
      std::shared_ptr<BabeObserver> babe_observer,
      std::shared_ptr<crypto::Hasher> hasher,
      std::shared_ptr<PeerManager> peer_manager)
      : host_(host),
        app_config_(app_config),
        stream_engine_(std::move(stream_engine)),
        block_tree_(std::move(block_tree)),
        storage_(std::move(storage)),
        babe_observer_(std::move(babe_observer)),
        hasher_(std::move(hasher)),
        peer_manager_(std::move(peer_manager)) {
    BOOST_ASSERT(stream_engine_ != nullptr);
    BOOST_ASSERT(block_tree_ != nullptr);
    BOOST_ASSERT(storage_ != nullptr);
    BOOST_ASSERT(babe_observer_ != nullptr);
    BOOST_ASSERT(hasher_ != nullptr);
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
            block_tree_->getBestContaining(last_finalized, boost::none);
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

    readStatus(
        stream,
        Direction::INCOMING,
        [wp = weak_from_this(),
         stream](outcome::result<std::shared_ptr<Stream>> stream_res) {
          auto self = wp.lock();
          if (not self) {
            stream->reset();
            return;
          }
          if (stream_res.has_value()) {
            std::ignore = self->stream_engine_->addIncoming(stream, self);
            self->peer_manager_->reserveStreams(stream->remotePeerId().value());
            SL_VERBOSE(self->log_,
                       "Fully established incoming {} stream with {}",
                       self->protocol_,
                       stream->remotePeerId().value().toBase58());
          } else {
            SL_VERBOSE(self->log_,
                       "Fail establishing incoming {} stream with {}: {}",
                       self->protocol_,
                       stream->remotePeerId().value().toBase58(),
                       stream_res.error().message());
          }
        });
  }

  void BlockAnnounceProtocol::newOutgoingStream(
      const PeerInfo &peer_info,
      std::function<void(outcome::result<std::shared_ptr<Stream>>)> &&cb) {
    SL_DEBUG(log_,
             "Connect for {} stream with {}",
             protocol_,
             peer_info.id.toBase58());

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
            SL_VERBOSE(
                self->log_,
                "Error happened while connection over {} stream with {}: {}",
                self->protocol_,
                peer_id.toBase58(),
                stream_res.error().message());
            cb(stream_res.as_failure());
            return;
          }

          auto &stream = stream_res.value();

          SL_DEBUG(self->log_,
                   "Established connection over {} stream with {}",
                   self->protocol_,
                   peer_id.toBase58());

          self->writeStatus(
              std::move(stream), Direction::OUTGOING, std::move(cb));
        });
  }

  void BlockAnnounceProtocol::readStatus(
      std::shared_ptr<Stream> stream,
      Direction direction,
      std::function<void(outcome::result<std::shared_ptr<Stream>>)> &&cb) {
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
            self->log_->error("Error while reading status: {}",
                              remote_status_res.error().message());
            stream->reset();
            cb(remote_status_res.as_failure());
            return;
          }
          auto &remote_status = remote_status_res.value();

          auto peer_id = stream->remotePeerId().value();
          SL_DEBUG(self->log_,
                   "Received status from peer_id={}",
                   peer_id.toBase58());
          self->peer_manager_->updatePeerStatus(peer_id, remote_status);
          auto self_status = self->createStatus();
          if (self_status) {
            if (self_status.value().best_block == remote_status.best_block
                && self_status.value().roles.flags.authority
                && remote_status.roles.flags.authority) {
              self->babe_observer_->onPeerSync();
            }
          }

          switch (direction) {
            case Direction::OUTGOING:
              std::ignore = self->stream_engine_->addOutgoing(stream, self);
              SL_VERBOSE(self->log_,
                         "Fully established outgoing {} stream with {}",
                         self->protocol_,
                         stream->remotePeerId().value().toBase58());
              cb(stream);
              break;
            case Direction::INCOMING:
              self->writeStatus(std::move(stream), direction, std::move(cb));
              break;
          }
        });
  }

  void BlockAnnounceProtocol::writeStatus(
      std::shared_ptr<Stream> stream,
      Direction direction,
      std::function<void(outcome::result<std::shared_ptr<Stream>>)> &&cb) {
    auto read_writer = std::make_shared<ScaleMessageReadWriter>(stream);

    auto status_res = createStatus();
    if (not status_res.has_value()) {
      cb(ProtocolError::CAN_NOT_CREATE_STATUS);
      return;
    }

    read_writer->write(
        status_res.value(),
        [stream, direction, wp = weak_from_this(), cb = std::move(cb)](
            auto &&write_res) mutable {
          auto self = wp.lock();
          if (not self) {
            stream->reset();
            cb(ProtocolError::GONE);
            return;
          }

          if (not write_res.has_value()) {
            SL_VERBOSE(self->log_,
                       "Error while writing own status: {}",
                       write_res.error().message());
            stream->reset();
            cb(write_res.as_failure());
            return;
          }

          switch (direction) {
            case Direction::OUTGOING:
              self->readStatus(std::move(stream), direction, std::move(cb));
              break;
            case Direction::INCOMING:
              self->readAnnounce(std::move(stream));
              cb(stream);
              break;
          }
        });
  }

  void BlockAnnounceProtocol::readAnnounce(std::shared_ptr<Stream> stream) {
    auto read_writer = std::make_shared<ScaleMessageReadWriter>(stream);

    read_writer->read<BlockAnnounce>(
        [stream, wp = weak_from_this()](auto &&block_announce_res) mutable {
          auto self = wp.lock();
          if (not self) {
            stream->reset();
            return;
          }

          if (not block_announce_res) {
            SL_VERBOSE(self->log_,
                       "Error while reading block announce: {}",
                       block_announce_res.error().message());
            stream->reset();
            return;
          }

          auto peer_id = stream->remotePeerId().value();
          auto &block_announce = block_announce_res.value();

          SL_VERBOSE(self->log_,
                     "Received block announce: block number {}",
                     block_announce.header.number);
          self->babe_observer_->onBlockAnnounce(peer_id, block_announce);

          auto hash = self->hasher_->blake2b_256(
              scale::encode(block_announce.header).value());

          self->peer_manager_->updatePeerStatus(
              stream->remotePeerId().value(),
              BlockInfo(block_announce.header.number, hash));

          self->readAnnounce(std::move(stream));
        });
  }

  void BlockAnnounceProtocol::writeAnnounce(
      std::shared_ptr<Stream> stream, const BlockAnnounce &block_announce) {
    std::function<void(outcome::result<std::shared_ptr<Stream>>)> cb =
        [](outcome::result<std::shared_ptr<Stream>>) {};

    auto read_writer = std::make_shared<ScaleMessageReadWriter>(stream);

    read_writer->write(block_announce,
                       [stream, wp = weak_from_this(), cb = std::move(cb)](
                           auto &&write_res) mutable {
                         auto self = wp.lock();
                         if (not self) {
                           stream->reset();
                           cb(ProtocolError::GONE);
                           return;
                         }

                         if (not write_res.has_value()) {
                           SL_VERBOSE(self->log_,
                                      "Error while writing block announce: {}",
                                      write_res.error().message());
                           stream->reset();
                           cb(write_res.as_failure());
                           return;
                         }

                         cb(stream);
                       });
  }

}  // namespace kagome::network
