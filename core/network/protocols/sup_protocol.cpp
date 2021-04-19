/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/protocols/sup_protocol.hpp"

#include "network/common.hpp"
#include "network/protocols/protocol_error.hpp"

namespace kagome::network {

  SupProtocol::SupProtocol(libp2p::Host &host,
                           std::shared_ptr<StreamEngine> stream_engine,
                           std::shared_ptr<blockchain::BlockTree> block_tree,
                           std::shared_ptr<blockchain::BlockStorage> storage,
                           std::shared_ptr<PeerManager> peer_manager)
      : host_(host),
        stream_engine_(std::move(stream_engine)),
        block_tree_(std::move(block_tree)),
        storage_(std::move(storage)),
        peer_manager_(std::move(peer_manager)) {
    BOOST_ASSERT(stream_engine_ != nullptr);
    BOOST_ASSERT(block_tree_ != nullptr);
    BOOST_ASSERT(storage_ != nullptr);
    BOOST_ASSERT(peer_manager_ != nullptr);
    const_cast<Protocol &>(protocol_) = kSupProtocol;
  }

  bool SupProtocol::start() {
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
      }
    });
    return true;
  }

  bool SupProtocol::stop() {
    return true;
  }

  outcome::result<Status> SupProtocol::createStatus() const {
    /// Roles
    Roles roles;
    // TODO(xDimon): Need to set actual role of node
    //  issue: https://github.com/soramitsu/kagome/issues/678
    roles.flags.full = 1;

    /// Best block info
    BlockInfo best_block;
    const auto &last_finalized = block_tree_->getLastFinalized().block_hash;
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

  void SupProtocol::onIncomingStream(std::shared_ptr<Stream> stream) {
    BOOST_ASSERT(stream->remotePeerId().has_value());

    auto status_res = createStatus();
    if (not status_res.has_value()) {
      stream->reset();
      return;
    }
    auto &status = status_res.value();

    writeStatus(
        stream,
        status,
        [wp = weak_from_this(),
         stream](outcome::result<std::shared_ptr<Stream>> stream_res) {
          auto self = wp.lock();
          if (not self) {
            stream->reset();
            return;
          }
          if (stream_res.has_value()) {
            std::ignore = self->stream_engine_->addIncoming(stream, self);
            SL_VERBOSE(self->log_,
                       "Fully established incoming {} stream with {}",
                       self->protocol_,
                       stream->remotePeerId().value().toBase58());
            self->readStatus(std::move(stream));
          } else {
            SL_VERBOSE(self->log_,
                       "Fail establishing incoming {} stream with {}: {}",
                       self->protocol_,
                       stream->remotePeerId().value().toBase58(),
                       stream_res.error().message());
          }
        });
  }

  void SupProtocol::newOutgoingStream(
      const PeerInfo &peer_info,
      std::function<void(outcome::result<std::shared_ptr<Stream>>)> &&cb) {
    host_.newStream(
        peer_info,
        protocol_,
        [wp = weak_from_this(), peer_id = peer_info.id, cb = std::move(cb)](
            auto &&stream_res) mutable {
          auto self = wp.lock();
          if (not self) {
            if (cb) cb(ProtocolError::GONE);
            return;
          }

          if (not stream_res.has_value()) {
            SL_VERBOSE(
                self->log_,
                "Error happened while connection over {} stream with {}: {}",
                self->protocol_,
                peer_id.toBase58(),
                stream_res.error().message());
            if (cb) cb(stream_res.as_failure());
            return;
          }
          auto &stream = stream_res.value();

          SL_DEBUG(self->log_,
                   "Established connection over {} stream with {}",
                   self->protocol_,
                   peer_id.toBase58());

          auto status_res = self->createStatus();
          if (not status_res.has_value()) {
            if (cb) cb(ProtocolError::CAN_NOT_CREATE_STATUS);
            return;
          }
          auto &status = status_res.value();

          self->writeStatus(stream, status, std::move(cb));
        });
  }

  void SupProtocol::readStatus(std::shared_ptr<Stream> stream) {
    auto read_writer = std::make_shared<ScaleMessageReadWriter>(stream);

    read_writer->read<Status>([stream, wp = weak_from_this()](
                                  auto &&remote_status_res) mutable {
      auto self = wp.lock();
      if (not self) {
        stream->reset();
        return;
      }

      if (not remote_status_res.has_value()) {
        SL_VERBOSE(self->log_,
                   "Error while reading status: {}",
                   remote_status_res.error().message());
        stream->reset();
        return;
      }

      auto peer_id = stream->remotePeerId().value();
      SL_VERBOSE(
          self->log_, "Received status from peer_id={}", peer_id.toBase58());

      self->peer_manager_->updatePeerStatus(peer_id, remote_status_res.value());

      self->readStatus(std::move(stream));
    });
  }

  void SupProtocol::writeStatus(
      std::shared_ptr<Stream> stream,
      const Status &status,
      std::function<void(outcome::result<std::shared_ptr<Stream>>)> &&cb) {
    auto read_writer = std::make_shared<ScaleMessageReadWriter>(stream);

    read_writer->write(status,
                       [stream = std::move(stream),
                        wp = weak_from_this(),
                        cb = std::move(cb)](auto &&write_res) mutable {
                         auto self = wp.lock();
                         if (not self) {
                           stream->reset();
                           if (cb) cb(ProtocolError::GONE);
                           return;
                         }

                         if (not write_res.has_value()) {
                           SL_VERBOSE(self->log_,
                                      "Error while writing own status: {}",
                                      write_res.error().message());
                           stream->reset();
                           if (cb) cb(write_res.as_failure());
                           return;
                         }

                         if (cb) cb(std::move(stream));
                       });
  }

}  // namespace kagome::network
