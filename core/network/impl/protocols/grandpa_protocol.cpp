/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/protocols/grandpa_protocol.hpp"

#include <libp2p/basic/scheduler.hpp>

#include "blockchain/genesis_block_hash.hpp"
#include "consensus/grandpa/grandpa_observer.hpp"
#include "network/common.hpp"
#include "network/notifications/encode.hpp"
#include "network/peer_manager.hpp"
#include "network/types/own_peer_info.hpp"
#include "utils/try.hpp"

namespace kagome::network {
  // https://github.com/paritytech/polkadot-sdk/blob/edf79aa972bcf2e043e18065a9bb860ecdbd1a6e/substrate/client/network-gossip/src/state_machine.rs#L40
  constexpr size_t kSeenCapacity = 8192;

  GrandpaProtocol::GrandpaProtocol(
      const notifications::Factory &notifications_factory,
      std::shared_ptr<crypto::Hasher> hasher,
      Roles roles,
      std::shared_ptr<consensus::grandpa::GrandpaObserver> grandpa_observer,
      const OwnPeerInfo &own_info,
      std::shared_ptr<PeerManager> peer_manager,
      const blockchain::GenesisBlockHash &genesis_hash,
      std::shared_ptr<libp2p::basic::Scheduler> scheduler)
      : log_{log::createLogger(kGrandpaProtocolName, "grandpa_protocol")},
        notifications_{notifications_factory.make(
            {make_protocols(
                kGrandpaProtocol, genesis_hash, kProtocolPrefixParitytech)},
            0,
            0)},
        hasher_{std::move(hasher)},
        roles_{roles},
        grandpa_observer_(std::move(grandpa_observer)),
        own_info_(own_info),
        peer_manager_(std::move(peer_manager)),
        scheduler_{std::move(scheduler)},
        seen_{kSeenCapacity} {}

  Buffer GrandpaProtocol::handshake() {
    return scale::encode(roles_).value();
  }

  bool GrandpaProtocol::onHandshake(const PeerId &peer_id,
                                    size_t,
                                    bool out,
                                    Buffer &&handshake) {
    TRY_FALSE(scale::decode<Roles>(handshake));
    if (out) {
      write(peer_id, rawMessage(last_neighbor_));
    }
    return true;
  }

  bool GrandpaProtocol::onMessage(const PeerId &peer_id,
                                  size_t,
                                  Buffer &&message_raw) {
    auto message = TRY_FALSE(scale::decode<GrandpaMessage>(message_raw));
    if (auto hash = rawMessageHash(message, message_raw)) {
      if (not seen_.add(peer_id, *hash)) {
        return true;
      }
    }
    visit_in_place(
        std::move(message),
        [&](network::GrandpaVote &&vote_message) {
          SL_VERBOSE(log_, "VoteMessage has received from {}", peer_id);
          auto info = peer_manager_->getPeerState(peer_id);
          grandpa_observer_->onVoteMessage(
              peer_id, compactFromRefToOwn(info), std::move(vote_message));
        },
        [&](FullCommitMessage &&commit_message) {
          SL_VERBOSE(log_, "CommitMessage has received from {}", peer_id);
          grandpa_observer_->onCommitMessage(peer_id,
                                             std::move(commit_message));
        },
        [&](GrandpaNeighborMessage &&neighbor_message) {
          if (peer_id != own_info_.id) {
            SL_VERBOSE(log_, "NeighborMessage has received from {}", peer_id);
            auto info = peer_manager_->getPeerState(peer_id);
            grandpa_observer_->onNeighborMessage(
                peer_id,
                compactFromRefToOwn(info),
                // NOLINTNEXTLINE(hicpp-move-const-arg,performance-move-const-arg)
                std::move(neighbor_message));
          }
        },
        [&](network::CatchUpRequest &&catch_up_request) {
          SL_VERBOSE(log_, "CatchUpRequest has received from {}", peer_id);
          auto info = peer_manager_->getPeerState(peer_id);
          grandpa_observer_->onCatchUpRequest(
              peer_id,
              compactFromRefToOwn(info),
              // NOLINTNEXTLINE(hicpp-move-const-arg,performance-move-const-arg)
              std::move(catch_up_request));
        },
        [&](network::CatchUpResponse &&catch_up_response) {
          SL_VERBOSE(log_, "CatchUpResponse has received from {}", peer_id);
          grandpa_observer_->onCatchUpResponse(peer_id,
                                               std::move(catch_up_response));
        });
    return true;
  }

  void GrandpaProtocol::onClose(const PeerId &peer_id) {
    seen_.remove(peer_id);
  }

  void GrandpaProtocol::start() {
    notifications_->start(weak_from_this());
  }

  void GrandpaProtocol::vote(
      network::GrandpaVote &&vote_message,
      std::optional<const libp2p::peer::PeerId> peer_id) {
    SL_DEBUG(log_,
             "Send vote message: grandpa round number {}",
             vote_message.round_number);

    auto filter = [&, &msg = vote_message](const PeerId &peer_id,
                                           const PeerState &info) {
      if (info.roles.isLight()) {
        return false;
      }

      if (not info.set_id.has_value() or not info.round_number.has_value()) {
        SL_DEBUG(log_,
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
        SL_DEBUG(log_,
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
            log_,
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
        SL_DEBUG(log_,
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

    auto raw_message = rawMessage(vote_message);
    if (not peer_id.has_value()) {
      broadcast(raw_message, filter);
    } else {
      write(*peer_id, std::move(raw_message));
    }
  }

  void GrandpaProtocol::neighbor(GrandpaNeighborMessage &&msg) {
    if (msg == last_neighbor_) {
      return;
    }
    auto set_changed = msg.voter_set_id != last_neighbor_.voter_set_id;
    last_neighbor_ = msg;

    SL_DEBUG(log_,
             "Send neighbor message: grandpa round number {}",
             msg.round_number);

    peer_manager_->updatePeerState(own_info_.id, msg);

    auto filter = [this,
                   set_changed,
                   set_id = msg.voter_set_id,
                   round_number = msg.round_number](const PeerId &peer_id) {
      auto info_opt = peer_manager_->getPeerState(peer_id);
      if (not info_opt.has_value()) {
        SL_DEBUG(log_,
                 "Neighbor message with set_id={} in round={} "
                 "has not been sent to {}: peer is not connected",
                 set_id,
                 round_number,
                 peer_id);
        return false;
      }
      const auto &info = info_opt.value().get();

      if (not set_changed and info.roles.isLight()) {
        return false;
      }

      return true;
    };

    auto raw_message = rawMessage(msg);
    notifications_->peersOut([&](const PeerId &peer_id, size_t) {
      if (filter(peer_id)) {
        write(peer_id, raw_message);
      }
      return true;
    });
  }

  void GrandpaProtocol::finalize(
      FullCommitMessage &&msg,
      std::optional<const libp2p::peer::PeerId> peer_id) {
    SL_DEBUG(log_, "Send commit message: grandpa round number {}", msg.round);

    auto filter = [this,
                   set_id = msg.set_id,
                   round_number = msg.round,
                   finalizing = msg.message.target_number](
                      const PeerId &peer_id, const PeerState &info) {
      if (not info.set_id.has_value() or not info.round_number.has_value()) {
        SL_DEBUG(log_,
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
        SL_DEBUG(log_,
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
            log_,
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
            log_,
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

    auto raw_message = rawMessage(msg);
    if (not peer_id.has_value()) {
      broadcast(raw_message, filter);
    } else {
      write(*peer_id, std::move(raw_message));
    }
  }

  void GrandpaProtocol::catchUpRequest(const libp2p::peer::PeerId &peer_id,
                                       CatchUpRequest &&catch_up_request) {
    SL_DEBUG(
        log_,
        "Send catch-up-request to {} beginning with grandpa round number {}",
        peer_id,
        catch_up_request.round_number);

    auto info_opt = peer_manager_->getPeerState(peer_id);
    if (not info_opt.has_value()) {
      SL_DEBUG(log_,
               "Catch-up-request with set_id={} in round={} "
               "has not been sent to {}: peer is not connected",
               catch_up_request.voter_set_id,
               catch_up_request.round_number,
               peer_id);
      return;
    }
    const auto &info = info_opt.value().get();

    if (not info.set_id.has_value() or not info.round_number.has_value()) {
      SL_DEBUG(log_,
               "Catch-up-request with set_id={} in round={} "
               "has not been sent to {}: set id or round number unknown",
               catch_up_request.voter_set_id,
               catch_up_request.round_number,
               peer_id);
      return;
    }

    // Impolite to send a catch up request to a peer in a new different Set ID.
    if (catch_up_request.voter_set_id != info.set_id) {
      SL_DEBUG(log_,
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
      SL_DEBUG(log_,
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
      SL_DEBUG(log_,
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
      SL_DEBUG(log_,
               "Catch-up-request with set_id={} in round={} "
               "has not been sent to {}: impolite to replay catch-up request",
               catch_up_request.voter_set_id,
               catch_up_request.round_number,
               peer_id);
      return;
    }

    scheduler_->schedule(
        [wp{weak_from_this()}, round_id, peer_id] {
          if (auto self = wp.lock()) {
            self->recent_catchup_requests_by_round_.erase(round_id);
            self->recent_catchup_requests_by_peer_.erase(peer_id);
          }
        },
        kRecentnessDuration);

    write(peer_id, rawMessage(catch_up_request));
  }

  void GrandpaProtocol::catchUpResponse(const libp2p::peer::PeerId &peer_id,
                                        CatchUpResponse &&catch_up_response) {
    SL_DEBUG(log_,
             "Send catch-up response: beginning with grandpa round number {}",
             catch_up_response.round_number);

    auto info_opt = peer_manager_->getPeerState(peer_id);
    if (not info_opt.has_value()) {
      SL_DEBUG(log_,
               "Catch-up-response with set_id={} in round={} "
               "has not been sent to {}: peer is not connected",
               catch_up_response.voter_set_id,
               catch_up_response.round_number,
               peer_id);
      return;
    }
    const auto &info = info_opt.value().get();

    if (not info.set_id.has_value() or not info.round_number.has_value()) {
      SL_DEBUG(log_,
               "Catch-up-response with set_id={} in round={} "
               "has not been sent to {}: set id or round number unknown",
               catch_up_response.voter_set_id,
               catch_up_response.round_number,
               peer_id);
      return;
    }

    /// Impolite to send a catch up request to a peer in a new different Set ID.
    if (catch_up_response.voter_set_id != info.set_id) {
      SL_DEBUG(log_,
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
      SL_DEBUG(log_,
               "Catch-up-response with set_id={} in round={} "
               "has not been sent to {}: is already not actual",
               catch_up_response.voter_set_id,
               catch_up_response.round_number,
               peer_id);
      return;
    }

    write(peer_id, rawMessage(catch_up_response));
  }

  std::optional<Hash256> GrandpaProtocol::rawMessageHash(
      const GrandpaMessage &message, BufferView message_raw) const {
    if (boost::get<GrandpaVote>(&message)
        or boost::get<FullCommitMessage>(&message)) {
      return hasher_->twox_256(message_raw);
    }
    return std::nullopt;
  }

  GrandpaProtocol::RawMessage GrandpaProtocol::rawMessage(
      const GrandpaMessage &message) const {
    auto message_raw = notifications::encode(message);
    auto hash = rawMessageHash(message, *message_raw);
    return {.raw = std::move(message_raw), .hash = hash};
  }

  bool GrandpaProtocol::write(const PeerId &peer_id, RawMessage raw_message) {
    if (not notifications_->peerOut(peer_id)) {
      return false;
    }
    if (raw_message.hash) {
      if (not seen_.add(peer_id, *raw_message.hash)) {
        return false;
      }
    }
    notifications_->write(peer_id, std::move(raw_message.raw));
    return true;
  }

  template <typename F>
  void GrandpaProtocol::broadcast(const RawMessage &raw_message,
                                  const F &predicate) {
    constexpr size_t kAuthorities = 4;
    constexpr size_t kAny = 4;
    std::deque<PeerId> authorities, any;
    notifications_->peersOut([&](const PeerId &peer_id, size_t) {
      if (auto info_ref = peer_manager_->getPeerState(peer_id)) {
        auto &info = info_ref->get();
        if (predicate(peer_id, info)) {
          (info.roles.isAuthority() ? authorities : any).emplace_back(peer_id);
        }
      }
      return true;
    });
    size_t need = 0;
    auto loop = [&](std::deque<PeerId> &peers) {
      while (not peers.empty() and need != 0) {
        auto &peer = peers.back();
        if (write(peer, raw_message)) {
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
