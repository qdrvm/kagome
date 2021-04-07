/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/protocols/block_announce_protocol.hpp"

#include <boost/assert.hpp>

#include "network/common.hpp"
#include "network/helpers/scale_message_read_writer.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::network, BlockAnnounceProtocol::Error, e) {
  using E = kagome::network::BlockAnnounceProtocol::Error;
  switch (e) {
    case E::CAN_NOT_CREATE_STATUS:
      return "Can not create status";
    case E::GONE:
      return "Protocol was switched off";
  }
  return "Unknown error";
}

namespace kagome::network {

  BlockAnnounceProtocol::BlockAnnounceProtocol(
      libp2p::Host &host,
      const application::ChainSpec &chain_spec,
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<blockchain::BlockStorage> storage,
      //      std::shared_ptr<PeerManager> peer_manager,
      std::shared_ptr<BabeObserver> babe_observer,
      std::shared_ptr<crypto::Hasher> hasher)
      : host_(host),
        block_tree_(std::move(block_tree)),
        storage_(std::move(storage)),
        //        peer_manager_(std::move(peer_manager)),
        babe_observer_(std::move(babe_observer)),
        hasher_(std::move(hasher)) {
    BOOST_ASSERT(block_tree_ != nullptr);
    BOOST_ASSERT(storage_ != nullptr);
    //    BOOST_ASSERT(peer_manager_ != nullptr);
    BOOST_ASSERT(babe_observer_ != nullptr);
    BOOST_ASSERT(hasher_ != nullptr);
    const_cast<Protocol &>(protocol_) =
        fmt::format(kBlockAnnouncesProtocol.data(), chain_spec.protocolId());
  }

  bool BlockAnnounceProtocol::start() {
    host_.setProtocolHandler(protocol_, [wp = weak_from_this()](auto &&stream) {
      if (auto self = wp.lock()) {
        if (auto peer_id = stream->remotePeerId()) {
          self->log_->trace("Handled {} protocol stream from: {}",
                            self->protocol_,
                            peer_id.value().toHex());
          self->onIncomingStream(std::forward<decltype(stream)>(stream));
          return;
        }
        self->log_->warn("Handled {} protocol stream from unknown peer",
                         self->protocol_);
      }
    });
    return true;
  }

  bool BlockAnnounceProtocol::stop() {
    return true;
  }

  outcome::result<Status> BlockAnnounceProtocol::createStatus() const {
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
      return Error::CAN_NOT_CREATE_STATUS;
    }

    /// Genesis hash
    BlockHash genesis_hash;
    if (auto genesis_res = storage_->getGenesisBlockHash();
        genesis_res.has_value()) {
      genesis_hash = std::move(genesis_res.value());
    } else {
      log_->error("Could not get genesis block hash: {}",
                  genesis_res.error().message());
      return Error::CAN_NOT_CREATE_STATUS;
    }

    return Status{
        .roles = roles, .best_block = best_block, .genesis_hash = genesis_hash};
  }

  void BlockAnnounceProtocol::onIncomingStream(std::shared_ptr<Stream> stream) {
    BOOST_ASSERT(stream->remotePeerId().has_value());

    readStatus(stream,
               Direction::INCOMING,
               [](outcome::result<std::shared_ptr<Stream>> stream_res) {
                 if (stream_res.has_value()) {
                   [] {}();
                 } else {
                   [] {}();
                 }
               });
  }

  void BlockAnnounceProtocol::newOutgoingStream(
      const PeerInfo &peer_info,
      std::function<void(outcome::result<std::shared_ptr<Stream>>)> &&cb) {
    host_.newStream(
        peer_info,
        protocol_,
        [wp = weak_from_this(), peer_id = peer_info.id, cb = std::move(cb)](
            auto &&stream_res) mutable {
          auto self = wp.lock();
          if (not self) {
            cb(Error::GONE);
            return;
          }

          if (not stream_res.has_value()) {
            cb(stream_res.as_failure());
            return;
          }

          auto &stream = stream_res.value();

          self->writeStatus(stream, Direction::OUTGOING, std::move(cb));
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
            cb(Error::GONE);
            return;
          }

          if (not remote_status_res.has_value()) {
            self->log_->error("Error while reading status: {}",
                              remote_status_res.error().message());
            stream->reset();
            cb(remote_status_res.as_failure());
            return;
          }

          //      auto peer_id = stream->remotePeerId().value();
          //      self->log_->debug("Received status from peer_id={}",
          //      peer_id.toBase58());
          //      self->peer_manager_->updatePeerStatus(peer_id,
          //      remote_status_res.value());

          switch (direction) {
            case Direction::OUTGOING:
              self->readAnnounce(stream);
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
      cb(Error::CAN_NOT_CREATE_STATUS);
      return;
    }

    read_writer->write(
        status_res.value(),
        [stream, direction, wp = weak_from_this(), cb = std::move(cb)](
            auto &&write_res) mutable {
          auto self = wp.lock();
          if (not self) {
            stream->reset();
            cb(Error::GONE);
            return;
          }

          if (not write_res.has_value()) {
            self->log_->error("Error while writing own status: {}",
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
            self->log_->error("Error while reading block announce: {}",
                              block_announce_res.error().message());
            stream->reset();
            return;
          }

          auto peer_id = stream->remotePeerId().value();
          auto &block_announce = block_announce_res.value();

          self->log_->info("Received block announce: block number {}",
                           block_announce.header.number);
          self->babe_observer_->onBlockAnnounce(peer_id, block_announce);

          auto hash = self->hasher_->blake2b_256(
              scale::encode(block_announce.header).value());

          //          self->peer_manager_->updatePeerStatus(
          //              stream->remotePeerId().value(),
          //              BlockInfo(block_announce.header.number, hash));

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
                           cb(Error::GONE);
                           return;
                         }

                         if (not write_res.has_value()) {
                           self->log_->error(
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
