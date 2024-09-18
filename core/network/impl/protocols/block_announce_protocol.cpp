/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/protocols/block_announce_protocol.hpp"

#include "blockchain/genesis_block_hash.hpp"
#include "network/common.hpp"
#include "network/helpers/scale_message_read_writer.hpp"
#include "network/impl/protocols/protocol_error.hpp"
#include "network/notifications/connect_and_handshake.hpp"
#include "network/notifications/handshake_and_read_messages.hpp"

namespace kagome::network {

  // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
  KAGOME_DEFINE_CACHE(BlockAnnounceProtocol);

  BlockAnnounceProtocol::BlockAnnounceProtocol(
      libp2p::Host &host,
      Roles roles,
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
        roles_{roles},
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

  BlockAnnounceHandshake BlockAnnounceProtocol::createHandshake() const {
    return BlockAnnounceHandshake{
        .roles = roles_,
        .best_block = block_tree_->bestBlock(),
        .genesis_hash = block_tree_->getGenesisBlockHash(),
    };
  }

  bool BlockAnnounceProtocol::onHandshake(
      const PeerId &peer, const BlockAnnounceHandshake &handshake) const {
    if (handshake.genesis_hash != block_tree_->getGenesisBlockHash()) {
      return false;
    }
    peer_manager_->updatePeerState(peer, handshake);
    observer_->onBlockAnnounceHandshake(peer, handshake);
    return true;
  }

  void BlockAnnounceProtocol::onIncomingStream(std::shared_ptr<Stream> stream) {
    BOOST_ASSERT(stream->remotePeerId().has_value());
    auto peer_id = stream->remotePeerId().value();
    auto on_handshake = [peer_id](std::shared_ptr<BlockAnnounceProtocol> self,
                                  std::shared_ptr<Stream> stream,
                                  BlockAnnounceHandshake handshake) {
      if (not self->onHandshake(peer_id, handshake)) {
        return false;
      }
      self->stream_engine_->addIncoming(stream, self);
      self->peer_manager_->reserveStreams(peer_id);
      self->peer_manager_->reserveStatusStreams(peer_id);
      self->peer_manager_->startPingingPeer(peer_id);
      return true;
    };
    auto on_message = [peer_id](std::shared_ptr<BlockAnnounceProtocol> self,
                                const BlockAnnounce &block_announce) {
      // Calculate and save hash, 'cause it's just received announce
      primitives::calculateBlockHash(block_announce.header, *self->hasher_);

      SL_VERBOSE(self->base_.logger(),
                 "Announce of block {} is received from {}",
                 block_announce.header.blockInfo(),
                 peer_id);

      self->peer_manager_->updatePeerState(peer_id, block_announce);
      self->observer_->onBlockAnnounce(peer_id, block_announce);
      return true;
    };
    notifications::handshakeAndReadMessages<BlockAnnounce>(
        weak_from_this(),
        std::move(stream),
        createHandshake(),
        std::move(on_handshake),
        std::move(on_message));
  }

  void BlockAnnounceProtocol::newOutgoingStream(
      const PeerId &peer_id,
      std::function<void(outcome::result<std::shared_ptr<Stream>>)> &&cb) {
    SL_DEBUG(base_.logger(),
             "Connect for {} stream with {}",
             protocolName(),
             peer_id);

    auto on_handshake = [peer_id, cb = std::move(cb)](
                            std::shared_ptr<BlockAnnounceProtocol> self,
                            outcome::result<notifications::ConnectAndHandshake<
                                BlockAnnounceHandshake>> r) mutable {
      if (not r) {
        cb(r.error());
        return;
      }
      if (not self->onHandshake(peer_id, std::get<2>(r.value()))) {
        cb(ProtocolError::GENESIS_NO_MATCH);
        return;
      }
      auto &stream = std::get<0>(r.value());
      self->stream_engine_->addOutgoing(stream, self);
      cb(std::move(stream));
    };
    notifications::connectAndHandshake(weak_from_this(),
                                       base_,
                                       peer_id,
                                       createHandshake(),
                                       std::move(on_handshake));
  }

  void BlockAnnounceProtocol::blockAnnounce(BlockAnnounce &&announce) {
    auto shared_msg =
        KAGOME_EXTRACT_SHARED_CACHE(BlockAnnounceProtocol, BlockAnnounce);
    (*shared_msg) = std::move(announce);
    const auto &hash = shared_msg->header.hash();

    SL_DEBUG(
        base_.logger(), "Send announce of block #{}", announce.header.number);

    stream_engine_->broadcast(
        shared_from_this(), shared_msg, [&](const PeerId &peer) {
          auto state = peer_manager_->getPeerState(peer);
          return state and state->get().known_blocks.add(hash);
        });
  }

}  // namespace kagome::network
