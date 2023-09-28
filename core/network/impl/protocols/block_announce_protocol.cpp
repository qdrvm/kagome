/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/protocols/block_announce_protocol.hpp"

#include "application/app_configuration.hpp"
#include "blockchain/genesis_block_hash.hpp"
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
      const blockchain::GenesisBlockHash &genesis_hash,
      std::shared_ptr<StreamEngine> stream_engine,
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<BlockAnnounceObserver> observer,
      std::shared_ptr<crypto::Hasher> hasher,
      std::shared_ptr<PeerManager> peer_manager)
      : base_(kBlockAnnounceProtocolName,
              host,
              make_protocols(kBlockAnnouncesProtocol, genesis_hash, chain_spec),
              log::createLogger(kBlockAnnounceProtocolName,
                                "block_announce_protocol")),
        app_config_(app_config),
        stream_engine_(std::move(stream_engine)),
        block_tree_(std::move(block_tree)),
        observer_(std::move(observer)),
        hasher_(std::move(hasher)),
        peer_manager_(std::move(peer_manager)) {
    BOOST_ASSERT(stream_engine_ != nullptr);
    BOOST_ASSERT(block_tree_ != nullptr);
    BOOST_ASSERT(observer_ != nullptr);
    BOOST_ASSERT(peer_manager_ != nullptr);
  }

  bool BlockAnnounceProtocol::start() {
    return base_.start(weak_from_this());
  }

  const ProtocolName &BlockAnnounceProtocol::protocolName() const {
    return base_.protocolName();
  }

  outcome::result<BlockAnnounceHandshake>
  BlockAnnounceProtocol::createHandshake() const {
    /// Roles
    Roles roles = app_config_.roles();

    /// Best block info
    auto best_block = block_tree_->bestBlock();

    auto &genesis_hash = block_tree_->getGenesisBlockHash();

    return BlockAnnounceHandshake{
        .roles = roles, .best_block = best_block, .genesis_hash = genesis_hash};
  }

  void BlockAnnounceProtocol::onIncomingStream(std::shared_ptr<Stream> stream) {
    BOOST_ASSERT(stream->remotePeerId().has_value());

    readHandshake(
        stream,
        Direction::INCOMING,
        [wp = weak_from_this(), stream](outcome::result<void> res) {
          auto self = wp.lock();
          if (not self) {
            stream->reset();
            return;
          }

          auto peer_id = stream->remotePeerId().value();

          if (not res.has_value()) {
            SL_VERBOSE(self->base_.logger(),
                       "Handshake failed on incoming {} stream with {}: {}",
                       self->protocolName(),
                       peer_id,
                       res.error());
            stream->reset();
            return;
          }

          res = self->stream_engine_->addIncoming(stream, self);
          if (not res.has_value()) {
            SL_VERBOSE(self->base_.logger(),
                       "Can't register incoming {} stream with {}: {}",
                       self->protocolName(),
                       peer_id,
                       res.error());
            stream->reset();
            return;
          }

          self->peer_manager_->reserveStreams(peer_id);
          self->peer_manager_->reserveStatusStreams(peer_id);
          self->peer_manager_->startPingingPeer(peer_id);

          SL_VERBOSE(self->base_.logger(),
                     "Fully established incoming {} stream with {}",
                     self->protocolName(),
                     peer_id);
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

          self->writeHandshake(std::move(stream_and_proto.stream),
                               Direction::OUTGOING,
                               std::move(cb2));
        });
  }

  void BlockAnnounceProtocol::readHandshake(
      std::shared_ptr<Stream> stream,
      Direction direction,
      std::function<void(outcome::result<void>)> &&cb) {
    auto read_writer = std::make_shared<ScaleMessageReadWriter>(stream);

    read_writer->read<BlockAnnounceHandshake>(
        [stream, direction, wp = weak_from_this(), cb = std::move(cb)](
            auto &&handshake_res) mutable {
          auto self = wp.lock();
          if (not self) {
            stream->reset();
            cb(ProtocolError::GONE);
            return;
          }

          BOOST_ASSERT_MSG(stream->remotePeerId().has_value(),
                           "peer_id must be known at this moment");
          auto peer_id = stream->remotePeerId().value();

          if (not handshake_res.has_value()) {
            SL_VERBOSE(self->base_.logger(),
                       "Can't read handshake from {} over {} stream: {}",
                       peer_id,
                       to_string(direction),
                       handshake_res.error());
            stream->reset();
            cb(handshake_res.as_failure());
            return;
          }
          auto &handshake = handshake_res.value();

          auto &genesis_hash = self->block_tree_->getGenesisBlockHash();

          if (handshake.genesis_hash != genesis_hash) {
            SL_VERBOSE(
                self->base_.logger(),
                "Error while processing handshake from {}: genesis no match",
                peer_id);
            stream->reset();
            cb(ProtocolError::GENESIS_NO_MATCH);
            return;
          }

          SL_TRACE(self->base_.logger(),
                   "Handshake has received from {} over {} stream: "
                   "roles {}, best block {}",
                   peer_id,
                   to_string(direction),
                   to_string(handshake.roles),
                   handshake.best_block);

          self->peer_manager_->updatePeerState(peer_id, handshake);

          switch (direction) {
            case Direction::OUTGOING:
              cb(outcome::success());
              break;
            case Direction::INCOMING:
              self->writeHandshake(
                  std::move(stream), Direction::INCOMING, std::move(cb));
              break;
          }

          self->observer_->onBlockAnnounceHandshake(peer_id, handshake);
        });
  }

  void BlockAnnounceProtocol::writeHandshake(
      std::shared_ptr<Stream> stream,
      Direction direction,
      std::function<void(outcome::result<void>)> &&cb) {
    auto read_writer = std::make_shared<ScaleMessageReadWriter>(stream);

    auto handshake_res = createHandshake();
    if (not handshake_res.has_value()) {
      stream->reset();
      cb(ProtocolError::CAN_NOT_CREATE_HANDSHAKE);
      return;
    }

    const auto &handshake = handshake_res.value();

    SL_TRACE(
        base_.logger(),
        "Handshake will sent to {} over {} stream: roles {}, best block {}",
        stream->remotePeerId().value(),
        to_string(direction),
        to_string(handshake.roles),
        handshake.best_block);

    read_writer->write(
        handshake,
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
                       "Can't send handshake to {} over {} stream: {}",
                       stream->remotePeerId().value(),
                       to_string(direction),
                       write_res.error());
            stream->reset();
            cb(write_res.as_failure());
            return;
          }

          SL_TRACE(self->base_.logger(),
                   "Handshake has sent to {} over {} stream",
                   stream->remotePeerId().value(),
                   to_string(direction));

          switch (direction) {
            case Direction::OUTGOING:
              self->readHandshake(
                  std::move(stream), Direction::OUTGOING, std::move(cb));
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

          BOOST_ASSERT_MSG(stream->remotePeerId().has_value(),
                           "peer_id must be known at this moment");
          auto peer_id = stream->remotePeerId().value();

          auto &block_announce = block_announce_res.value();

          SL_VERBOSE(self->base_.logger(),
                     "Announce of block #{} is received from {}",
                     block_announce.header.number,
                     peer_id);

          self->observer_->onBlockAnnounce(peer_id, block_announce);

          self->peer_manager_->updatePeerState(stream->remotePeerId().value(),
                                               block_announce);

          self->readAnnounce(std::move(stream));
        });
  }

  void BlockAnnounceProtocol::blockAnnounce(BlockAnnounce &&announce) {
    auto hash = hasher_->blake2b_256(scale::encode(announce.header).value());

    auto shared_msg =
        KAGOME_EXTRACT_SHARED_CACHE(BlockAnnounceProtocol, BlockAnnounce);
    (*shared_msg) = std::move(announce);

    SL_DEBUG(
        base_.logger(), "Send announce of block #{}", announce.header.number);

    stream_engine_->broadcast(
        shared_from_this(), shared_msg, [&](const PeerId &peer) {
          auto state = peer_manager_->getPeerState(peer);
          return state and state->get().known_blocks.add(hash);
        });
  }

}  // namespace kagome::network
