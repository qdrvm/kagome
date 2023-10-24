/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/protocols/grandpa_protocol.hpp"

#include <libp2p/connection/loopback_stream.hpp>

#include "blockchain/block_tree.hpp"
#include "blockchain/genesis_block_hash.hpp"
#include "network/common.hpp"
#include "network/impl/protocols/protocol_error.hpp"
#include "network/notifications/connect_and_handshake.hpp"
#include "network/notifications/handshake_and_read_messages.hpp"
#include "network/peer_manager.hpp"
#include "network/types/grandpa_message.hpp"
#include "network/types/roles.hpp"

namespace kagome::network {
  using libp2p::connection::LoopbackStream;

  KAGOME_DEFINE_CACHE(GrandpaProtocol);

  GrandpaProtocol::GrandpaProtocol(
      libp2p::Host &host,
      std::shared_ptr<crypto::Hasher> hasher,
      std::shared_ptr<boost::asio::io_context> io_context,
      Roles roles,
      std::shared_ptr<consensus::grandpa::GrandpaObserver> grandpa_observer,
      const OwnPeerInfo &own_info,
      std::shared_ptr<StreamEngine> stream_engine,
      std::shared_ptr<PeerManager> peer_manager,
      const blockchain::GenesisBlockHash &genesis_hash,
      std::shared_ptr<libp2p::basic::Scheduler> scheduler)
      : base_(kGrandpaProtocolName,
              host,
              make_protocols(
                  kGrandpaProtocol, genesis_hash, kProtocolPrefixParitytech),
              log::createLogger(kGrandpaProtocolName, "grandpa_protocol")),
        hasher_{std::move(hasher)},
        io_context_(std::move(io_context)),
        roles_{roles},
        grandpa_observer_(std::move(grandpa_observer)),
        own_info_(own_info),
        stream_engine_(std::move(stream_engine)),
        peer_manager_(std::move(peer_manager)),
        scheduler_(std::move(scheduler)) {}

  bool GrandpaProtocol::start() {
    auto stream = std::make_shared<LoopbackStream>(own_info_, io_context_);
    stream_engine_->addBidirectional(stream, shared_from_this());
    auto on_message = [weak = weak_from_this(),
                       peer_id = own_info_.id](GrandpaMessage message) {
      auto self = weak.lock();
      if (not self) {
        return false;
      }
      self->onMessage(peer_id, std::move(message));
      return true;
    };
    notifications::readMessages<GrandpaMessage>(
        stream,
        std::make_shared<libp2p::basic::MessageReadWriterUvarint>(stream),
        std::move(on_message));

    return base_.start(weak_from_this());
  }

  void GrandpaProtocol::stop() {
    stream_engine_->del(own_info_.id, shared_from_this());
  }

  const ProtocolName &GrandpaProtocol::protocolName() const {
    return base_.protocolName();
  }

  void GrandpaProtocol::onIncomingStream(std::shared_ptr<Stream> stream) {
    BOOST_ASSERT(stream->remotePeerId().has_value());
    auto on_handshake = [](std::shared_ptr<GrandpaProtocol> self,
                           std::shared_ptr<Stream> stream,
                           Roles) {
      self->stream_engine_->addIncoming(stream, self);
      return true;
    };
    auto on_message = [peer_id = stream->remotePeerId().value()](
                          std::shared_ptr<GrandpaProtocol> self,
                          GrandpaMessage message) {
      self->onMessage(peer_id, std::move(message));
      return true;
    };
    notifications::handshakeAndReadMessages<GrandpaMessage>(
        weak_from_this(),
        std::move(stream),
        roles_,
        std::move(on_handshake),
        std::move(on_message));
  }

  void GrandpaProtocol::newOutgoingStream(
      const PeerInfo &peer_info,
      std::function<void(outcome::result<std::shared_ptr<Stream>>)> &&cb) {
    auto on_handshake =
        [cb = std::move(cb)](
            std::shared_ptr<GrandpaProtocol> self,
            outcome::result<notifications::ConnectAndHandshake<Roles>>
                r) mutable {
          if (not r) {
            cb(r.error());
            return;
          }
          auto &stream = std::get<0>(r.value());
          self->stream_engine_->addOutgoing(stream, self);
          auto shared_msg =
              KAGOME_EXTRACT_SHARED_CACHE(GrandpaProtocol, GrandpaMessage);
          (*shared_msg) = self->last_neighbor_;
          self->stream_engine_->send(
              stream->remotePeerId().value(), self, std::move(shared_msg));
          cb(std::move(stream));
        };
    notifications::connectAndHandshake(
        weak_from_this(), base_, peer_info, roles_, std::move(on_handshake));
  }

  void GrandpaProtocol::onMessage(const PeerId &peer_id,
                                  GrandpaMessage message) {
    auto hash = getHash(message);
    visit_in_place(
        std::move(message),
        [&](network::GrandpaVote &&vote_message) {
          SL_VERBOSE(
              base_.logger(), "VoteMessage has received from {}", peer_id);
          auto info = peer_manager_->getPeerState(peer_id);
          grandpa_observer_->onVoteMessage(
              std::nullopt, peer_id, compactFromRefToOwn(info), vote_message);
          addKnown(peer_id, hash);
        },
        [&](FullCommitMessage &&commit_message) {
          SL_VERBOSE(
              base_.logger(), "CommitMessage has received from {}", peer_id);
          grandpa_observer_->onCommitMessage(
              std::nullopt, peer_id, commit_message);
          addKnown(peer_id, hash);
        },
        [&](GrandpaNeighborMessage &&neighbor_message) {
          if (peer_id != own_info_.id) {
            SL_VERBOSE(base_.logger(),
                       "NeighborMessage has received from {}",
                       peer_id);
            auto info = peer_manager_->getPeerState(peer_id);
            grandpa_observer_->onNeighborMessage(peer_id,
                                                 compactFromRefToOwn(info),
                                                 std::move(neighbor_message));
          }
        },
        [&](network::CatchUpRequest &&catch_up_request) {
          SL_VERBOSE(
              base_.logger(), "CatchUpRequest has received from {}", peer_id);
          auto info = peer_manager_->getPeerState(peer_id);
          grandpa_observer_->onCatchUpRequest(
              peer_id, compactFromRefToOwn(info), std::move(catch_up_request));
        },
        [&](network::CatchUpResponse &&catch_up_response) {
          SL_VERBOSE(
              base_.logger(), "CatchUpResponse has received from {}", peer_id);
          grandpa_observer_->onCatchUpResponse(
              std::nullopt, peer_id, catch_up_response);
        });
  }

  void GrandpaProtocol::vote(
      network::GrandpaVote &&vote_message,
      std::optional<const libp2p::peer::PeerId> peer_id) {
    SL_DEBUG(base_.logger(),
             "Send vote message: grandpa round number {}",
             vote_message.round_number);

    auto filter = [&, &msg = vote_message](const PeerId &peer_id,
                                           const PeerState &info) {
      if (info.roles.flags.light != 0) {
        return false;
      }

      if (not info.set_id.has_value() or not info.round_number.has_value()) {
        SL_DEBUG(base_.logger(),
                 "Vote signed by {} with set_id={} in round={} "
                 "has not been sent to {}: set id or round number unknown",
                 msg.id(),
                 msg.counter,
                 msg.round_number,
                 peer_id);
        return false;
      }

      // If a peer is at a given voter set, it is impolite to send messages
      // from an earlier voter set. It is extremely impolite to send messages
      // from a future voter set.
      if (msg.counter != info.set_id) {
        SL_DEBUG(base_.logger(),
                 "Vote signed by {} with set_id={} in round={} "
                 "has not been sent to {} as impolite: their set id is {}",
                 msg.id(),
                 msg.counter,
                 msg.round_number,
                 peer_id,
                 info.set_id.value());
        return false;
      }

      // only r-1 ... r+1
      if (msg.round_number + 1 < info.round_number.value()) {
        SL_DEBUG(
            base_.logger(),
            "Vote signed by {} with set_id={} in round={} "
            "has not been sent to {} as impolite: their round is already {}",
            msg.id(),
            msg.counter,
            msg.round_number,
            peer_id,
            info.round_number.value());
        return false;
      }

      if (msg.round_number > info.round_number.value() + 1) {
        SL_DEBUG(base_.logger(),
                 "Vote signed by {} with set_id={} in round={} "
                 "has not been sent to {} as impolite: their round is old: {}",
                 msg.id(),
                 msg.counter,
                 msg.round_number,
                 peer_id,
                 info.round_number.value());
        return false;
      }

      return true;
    };

    auto shared_msg =
        KAGOME_EXTRACT_SHARED_CACHE(GrandpaProtocol, GrandpaMessage);
    (*shared_msg) = GrandpaMessage(std::move(vote_message));

    if (not peer_id.has_value()) {
      broadcast(std::move(shared_msg), filter);
    } else {
      addKnown(*peer_id, getHash(*shared_msg));
      stream_engine_->send(
          peer_id.value(), shared_from_this(), std::move(shared_msg));
    };
  }

  void GrandpaProtocol::neighbor(GrandpaNeighborMessage &&msg) {
    if (msg == last_neighbor_) {
      return;
    }
    auto set_changed = msg.voter_set_id != last_neighbor_.voter_set_id;
    last_neighbor_ = msg;

    SL_DEBUG(base_.logger(),
             "Send neighbor message: grandpa round number {}",
             msg.round_number);

    peer_manager_->updatePeerState(own_info_.id, msg);

    auto filter = [this,
                   set_changed,
                   set_id = msg.voter_set_id,
                   round_number = msg.round_number](const PeerId &peer_id) {
      auto info_opt = peer_manager_->getPeerState(peer_id);
      if (not info_opt.has_value()) {
        SL_DEBUG(base_.logger(),
                 "Neighbor message with set_id={} in round={} "
                 "has not been sent to {}: peer is not connected",
                 set_id,
                 round_number,
                 peer_id);
        return false;
      }
      const auto &info = info_opt.value().get();

      if (not set_changed and info.roles.flags.light) {
        return false;
      }

      return true;
    };

    auto shared_msg =
        KAGOME_EXTRACT_SHARED_CACHE(GrandpaProtocol, GrandpaMessage);
    (*shared_msg) = GrandpaMessage(msg);

    stream_engine_->broadcast<GrandpaMessage>(
        shared_from_this(), shared_msg, filter);
  }

  void GrandpaProtocol::finalize(
      FullCommitMessage &&msg,
      std::optional<const libp2p::peer::PeerId> peer_id) {
    SL_DEBUG(base_.logger(),
             "Send commit message: grandpa round number {}",
             msg.round);

    auto filter = [this,
                   set_id = msg.set_id,
                   round_number = msg.round,
                   finalizing = msg.message.target_number](
                      const PeerId &peer_id, const PeerState &info) {
      if (not info.set_id.has_value() or not info.round_number.has_value()) {
        SL_DEBUG(base_.logger(),
                 "Commit with set_id={} in round={} "
                 "has not been sent to {}: set id or round number unknown",
                 set_id,
                 round_number,
                 peer_id);
        return false;
      }

      // It is especially impolite to send commits which are invalid, or from
      // a different Set ID than the receiving peer has indicated.
      if (set_id != info.set_id) {
        SL_DEBUG(base_.logger(),
                 "Commit with set_id={} in round={} "
                 "has not been sent to {} as impolite: their set id is {}",
                 set_id,
                 round_number,
                 peer_id,
                 info.set_id.value());
        return false;
      }

      // Don't send commit if that has not actual for remote peer already
      if (round_number < info.round_number.value()) {
        SL_DEBUG(
            base_.logger(),
            "Commit with set_id={} in round={} "
            "has not been sent to {} as impolite: their round is already {}",
            set_id,
            round_number,
            peer_id,
            info.round_number.value());
        return false;
      }

      // It is impolite to send commits which are earlier than the last commit
      // sent.
      if (finalizing < info.last_finalized) {
        SL_DEBUG(
            base_.logger(),
            "Commit with set_id={} in round={} "
            "has not been sent to {} as impolite: their round is already {}",
            set_id,
            round_number,
            peer_id,
            info.round_number.value());
        return false;
      }

      return true;
    };

    auto shared_msg =
        KAGOME_EXTRACT_SHARED_CACHE(GrandpaProtocol, GrandpaMessage);
    (*shared_msg) = GrandpaMessage(std::move(msg));

    if (not peer_id.has_value()) {
      broadcast(std::move(shared_msg), filter);
    } else {
      addKnown(*peer_id, getHash(*shared_msg));
      stream_engine_->send(
          peer_id.value(), shared_from_this(), std::move(shared_msg));
    }
  }

  void GrandpaProtocol::catchUpRequest(const libp2p::peer::PeerId &peer_id,
                                       CatchUpRequest &&catch_up_request) {
    SL_DEBUG(
        base_.logger(),
        "Send catch-up-request to {} beginning with grandpa round number {}",
        peer_id,
        catch_up_request.round_number);

    auto info_opt = peer_manager_->getPeerState(peer_id);
    if (not info_opt.has_value()) {
      SL_DEBUG(base_.logger(),
               "Catch-up-request with set_id={} in round={} "
               "has not been sent to {}: peer is not connected",
               catch_up_request.voter_set_id,
               catch_up_request.round_number,
               peer_id);
      return;
    }
    const auto &info = info_opt.value().get();

    if (not info.set_id.has_value() or not info.round_number.has_value()) {
      SL_DEBUG(base_.logger(),
               "Catch-up-request with set_id={} in round={} "
               "has not been sent to {}: set id or round number unknown",
               catch_up_request.voter_set_id,
               catch_up_request.round_number,
               peer_id);
      return;
    }

    // Impolite to send a catch up request to a peer in a new different Set ID.
    if (catch_up_request.voter_set_id != info.set_id) {
      SL_DEBUG(base_.logger(),
               "Catch-up-request with set_id={} in round={} "
               "has not been sent to {}: different set id",
               catch_up_request.voter_set_id,
               catch_up_request.round_number,
               peer_id);
      return;
    }

    // It is impolite to send a catch-up request for a round `R` to a peer
    // whose announced view is behind `R`.
    if (catch_up_request.round_number < info.round_number.value() - 1) {
      SL_DEBUG(base_.logger(),
               "Catch-up-request with set_id={} in round={} "
               "has not been sent to {}: too old round for requested",
               catch_up_request.voter_set_id,
               catch_up_request.round_number,
               peer_id);
      return;
    }

    auto round_id = std::tuple(info.round_number.value(), info.set_id.value());

    auto [iter_by_round, ok_by_round] =
        recent_catchup_requests_by_round_.emplace(round_id);

    if (not ok_by_round) {
      SL_DEBUG(base_.logger(),
               "Catch-up-request with set_id={} in round={} "
               "has not been sent to {}: "
               "the same catch-up request had sent to another peer",
               catch_up_request.voter_set_id,
               catch_up_request.round_number,
               peer_id);
      return;
    }

    auto [iter_by_peer, ok_by_peer] =
        recent_catchup_requests_by_peer_.emplace(peer_id);

    // It is impolite to replay a catch-up request
    if (not ok_by_peer) {
      recent_catchup_requests_by_round_.erase(iter_by_round);
      SL_DEBUG(base_.logger(),
               "Catch-up-request with set_id={} in round={} "
               "has not been sent to {}: impolite to replay catch-up request",
               catch_up_request.voter_set_id,
               catch_up_request.round_number,
               peer_id);
      return;
    }

    scheduler_->schedule(
        [wp = weak_from_this(), round_id, peer_id] {
          if (auto self = wp.lock()) {
            self->recent_catchup_requests_by_round_.erase(round_id);
            self->recent_catchup_requests_by_peer_.erase(peer_id);
          }
        },
        kRecentnessDuration);

    auto shared_msg =
        KAGOME_EXTRACT_SHARED_CACHE(GrandpaProtocol, GrandpaMessage);
    (*shared_msg) = GrandpaMessage(std::move(catch_up_request));

    stream_engine_->send(peer_id, shared_from_this(), std::move(shared_msg));
  }

  void GrandpaProtocol::catchUpResponse(const libp2p::peer::PeerId &peer_id,
                                        CatchUpResponse &&catch_up_response) {
    SL_DEBUG(base_.logger(),
             "Send catch-up response: beginning with grandpa round number {}",
             catch_up_response.round_number);

    auto info_opt = peer_manager_->getPeerState(peer_id);
    if (not info_opt.has_value()) {
      SL_DEBUG(base_.logger(),
               "Catch-up-response with set_id={} in round={} "
               "has not been sent to {}: peer is not connected",
               catch_up_response.voter_set_id,
               catch_up_response.round_number,
               peer_id);
      return;
    }
    const auto &info = info_opt.value().get();

    if (not info.set_id.has_value() or not info.round_number.has_value()) {
      SL_DEBUG(base_.logger(),
               "Catch-up-response with set_id={} in round={} "
               "has not been sent to {}: set id or round number unknown",
               catch_up_response.voter_set_id,
               catch_up_response.round_number,
               peer_id);
      return;
    }

    /// Impolite to send a catch up request to a peer in a new different Set ID.
    if (catch_up_response.voter_set_id != info.set_id) {
      SL_DEBUG(base_.logger(),
               "Catch-up-response with set_id={} in round={} "
               "has not been sent to {}: {} set id",
               catch_up_response.voter_set_id,
               catch_up_response.round_number,
               peer_id,
               info.set_id.has_value() ? "different" : "unknown");
      return;
    }

    /// Avoid sending useless response (if peer is already caught up)
    if (catch_up_response.round_number < info.round_number) {
      SL_DEBUG(base_.logger(),
               "Catch-up-response with set_id={} in round={} "
               "has not been sent to {}: is already not actual",
               catch_up_response.voter_set_id,
               catch_up_response.round_number,
               peer_id);
      return;
    }

    auto shared_msg =
        KAGOME_EXTRACT_SHARED_CACHE(GrandpaProtocol, GrandpaMessage);
    (*shared_msg) = GrandpaMessage(std::move(catch_up_response));

    stream_engine_->send(peer_id, shared_from_this(), std::move(shared_msg));
  }

  common::Hash256 GrandpaProtocol::getHash(
      const GrandpaMessage &message) const {
    return hasher_->twox_256(scale::encode(message).value());
  }

  bool GrandpaProtocol::addKnown(const PeerId &peer,
                                 const common::Hash256 &hash) {
    auto info = peer_manager_->getPeerState(peer);
    return info and info->get().known_grandpa_messages.add(hash);
  }

  template <typename F>
  void GrandpaProtocol::broadcast(std::shared_ptr<GrandpaMessage> message,
                                  const F &predicate) {
    constexpr size_t kAuthorities = 4;
    constexpr size_t kAny = 4;
    std::deque<PeerId> authorities, any;
    stream_engine_->forEachPeer(shared_from_this(), [&](const PeerId &peer) {
      if (auto info_ref = peer_manager_->getPeerState(peer)) {
        auto &info = info_ref->get();
        if (not predicate(peer, info)) {
          return;
        }
        (info.roles.flags.authority != 0 ? authorities : any)
            .emplace_back(peer);
      }
    });
    auto hash = getHash(*message);
    size_t need = 0;
    auto loop = [&](std::deque<PeerId> &peers) {
      while (not peers.empty() and need != 0) {
        auto &peer = peers.back();
        if (addKnown(peer, hash)) {
          stream_engine_->send(peer, shared_from_this(), message);
          --need;
        }
        peers.pop_back();
      }
    };
    std::shuffle(authorities.begin(), authorities.end(), random_);
    need += kAuthorities;
    loop(authorities);
    any.insert(any.end(),
               std::make_move_iterator(authorities.begin()),
               std::make_move_iterator(authorities.end()));
    std::shuffle(any.begin(), any.end(), random_);
    need += kAny;
    loop(any);
  }
}  // namespace kagome::network
