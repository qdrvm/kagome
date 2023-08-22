/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/protocols/grandpa_protocol.hpp"

#include <libp2p/connection/loopback_stream.hpp>

#include "blockchain/block_tree.hpp"
#include "blockchain/genesis_block_hash.hpp"
#include "network/common.hpp"
#include "network/helpers/peer_id_formatter.hpp"
#include "network/impl/protocols/protocol_error.hpp"
#include "network/peer_manager.hpp"
#include "network/types/grandpa_message.hpp"
#include "network/types/roles.hpp"

namespace kagome::network {
  using libp2p::connection::LoopbackStream;

  KAGOME_DEFINE_CACHE(GrandpaProtocol);

  GrandpaProtocol::GrandpaProtocol(
      libp2p::Host &host,
      std::shared_ptr<boost::asio::io_context> io_context,
      const application::AppConfiguration &app_config,
      std::shared_ptr<consensus::grandpa::GrandpaObserver> grandpa_observer,
      const OwnPeerInfo &own_info,
      std::shared_ptr<StreamEngine> stream_engine,
      std::shared_ptr<PeerManager> peer_manager,
      const blockchain::GenesisBlockHash &genesis_hash,
      std::shared_ptr<libp2p::basic::Scheduler> scheduler)
      : base_(kGrandpaProtocolName,
              host,
              make_protocols(kGrandpaProtocol, genesis_hash, "paritytech"),
              log::createLogger(kGrandpaProtocolName, "grandpa_protocol")),
        io_context_(std::move(io_context)),
        app_config_(app_config),
        grandpa_observer_(std::move(grandpa_observer)),
        own_info_(own_info),
        stream_engine_(std::move(stream_engine)),
        peer_manager_(std::move(peer_manager)),
        scheduler_(std::move(scheduler)) {}

  bool GrandpaProtocol::start() {
    auto stream = std::make_shared<LoopbackStream>(own_info_, io_context_);
    auto res = stream_engine_->addBidirectional(stream, shared_from_this());
    if (not res.has_value()) {
      return false;
    }
    read(std::move(stream));
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

          SL_VERBOSE(self->base_.logger(),
                     "Fully established incoming {} stream with {}",
                     self->protocolName(),
                     peer_id);
        });
  }

  void GrandpaProtocol::newOutgoingStream(
      const PeerInfo &peer_info,
      std::function<void(outcome::result<std::shared_ptr<Stream>>)> &&cb) {
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
          auto &stream_and_proto = stream_res.value();

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

            // Send neighbor message first
            SL_DEBUG(self->base_.logger(),
                     "Send initial neighbor message: grandpa round number {}",
                     self->last_neighbor_.round_number);

            auto shared_msg =
                KAGOME_EXTRACT_SHARED_CACHE(GrandpaProtocol, GrandpaMessage);
            (*shared_msg) = self->last_neighbor_;

            self->stream_engine_->send(
                stream->remotePeerId().value(), self, std::move(shared_msg));

            cb(std::move(stream));
          };

          self->writeHandshake(std::move(stream_and_proto.stream),
                               Direction::OUTGOING,
                               std::move(cb2));
        });
  }

  void GrandpaProtocol::readHandshake(
      std::shared_ptr<Stream> stream,
      Direction direction,
      std::function<void(outcome::result<void>)> &&cb) {
    auto read_writer = std::make_shared<ScaleMessageReadWriter>(stream);

    read_writer->read<Roles>(
        [stream, direction, wp = weak_from_this(), cb = std::move(cb)](
            auto &&remote_roles_res) mutable {
          auto self = wp.lock();
          if (not self) {
            stream->reset();
            cb(ProtocolError::GONE);
            return;
          }

          if (not remote_roles_res.has_value()) {
            SL_VERBOSE(self->base_.logger(),
                       "Can't read handshake from {}: {}",
                       stream->remotePeerId().value(),
                       remote_roles_res.error());
            stream->reset();
            cb(remote_roles_res.as_failure());
            return;
          }
          [[maybe_unused]] auto &remote_roles = remote_roles_res.value();

          SL_TRACE(self->base_.logger(),
                   "Handshake has received from {}; roles={}",
                   stream->remotePeerId().value(),
                   to_string(remote_roles));

          switch (direction) {
            case Direction::OUTGOING:
              cb(outcome::success());
              break;
            case Direction::INCOMING:
              self->writeHandshake(
                  std::move(stream), Direction::INCOMING, std::move(cb));
              break;
          }
        });
  }

  void GrandpaProtocol::writeHandshake(
      std::shared_ptr<Stream> stream,
      Direction direction,
      std::function<void(outcome::result<void>)> &&cb) {
    auto read_writer = std::make_shared<ScaleMessageReadWriter>(stream);

    Roles roles = app_config_.roles();

    read_writer->write(roles,
                       [stream = std::move(stream),
                        roles,
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
                                  "Handshake has sent to {}; roles={}",
                                  stream->remotePeerId().value(),
                                  to_string(roles));

                         switch (direction) {
                           case Direction::OUTGOING:
                             self->readHandshake(
                                 std::move(stream), direction, std::move(cb));
                             break;
                           case Direction::INCOMING:
                             cb(outcome::success());
                             self->read(std::move(stream));
                             break;
                         }
                       });
  }

  void GrandpaProtocol::read(std::shared_ptr<Stream> stream) {
    auto read_writer = std::make_shared<ScaleMessageReadWriter>(stream);

    read_writer->read<GrandpaMessage>(
        [stream = std::move(stream),
         wp = weak_from_this()](auto &&grandpa_message_res) mutable {
          auto self = wp.lock();
          if (not self) {
            stream->reset();
            return;
          }

          if (not grandpa_message_res.has_value()) {
            SL_VERBOSE(self->base_.logger(),
                       "Can't read grandpa message from {}: {}",
                       stream->remotePeerId().value(),
                       grandpa_message_res.error());
            stream->reset();
            return;
          }

          auto peer_id = stream->remotePeerId().value();
          visit_in_place(
              std::move(grandpa_message_res.value()),
              [&](network::GrandpaVote &&vote_message) {
                SL_VERBOSE(self->base_.logger(),
                           "VoteMessage has received from {}",
                           peer_id);
                self->grandpa_observer_->onVoteMessage(
                    std::nullopt, peer_id, vote_message);
              },
              [&](FullCommitMessage &&commit_message) {
                SL_VERBOSE(self->base_.logger(),
                           "CommitMessage has received from {}",
                           peer_id);
                self->grandpa_observer_->onCommitMessage(
                    std::nullopt, peer_id, commit_message);
              },
              [&](GrandpaNeighborMessage &&neighbor_message) {
                if (peer_id != self->own_info_.id) {
                  SL_VERBOSE(self->base_.logger(),
                             "NeighborMessage has received from {}",
                             peer_id);
                  self->grandpa_observer_->onNeighborMessage(
                      peer_id, std::move(neighbor_message));
                }
              },
              [&](network::CatchUpRequest &&catch_up_request) {
                SL_VERBOSE(self->base_.logger(),
                           "CatchUpRequest has received from {}",
                           peer_id);
                self->grandpa_observer_->onCatchUpRequest(
                    peer_id, std::move(catch_up_request));
              },
              [&](network::CatchUpResponse &&catch_up_response) {
                SL_VERBOSE(self->base_.logger(),
                           "CatchUpResponse has received from {}",
                           peer_id);
                self->grandpa_observer_->onCatchUpResponse(
                    std::nullopt, peer_id, catch_up_response);
              });
          self->read(std::move(stream));
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


  template <typename F>
  void GrandpaProtocol::broadcast(std::shared_ptr<GrandpaMessage> message,
                                  const F &predicate) {
    constexpr size_t kAuthorities = 4;
    constexpr size_t kAny = 4;
    std::vector<PeerId> authorities, any;
    peer_manager_->forEachPeer([&](const PeerId &peer) {
      if (auto info_ref = peer_manager_->getPeerState(peer)) {
        auto &info = info_ref->get();
        if (not predicate(peer, info)) {
          return;
        }
        (info.roles.flags.authority != 0 ? authorities : any)
            .emplace_back(peer);
      }
    });
    std::shuffle(authorities.begin(), authorities.end(), random_);
    size_t need = 0;
    for (need += kAuthorities; not authorities.empty() and need != 0; --need) {
      stream_engine_->send(authorities.back(), shared_from_this(), message);
      authorities.pop_back();
    }
    any.insert(any.end(),
               std::make_move_iterator(authorities.begin()),
               std::make_move_iterator(authorities.end()));
    std::shuffle(any.begin(), any.end(), random_);
    for (need += kAny; not any.empty() and need != 0; --need) {
      stream_engine_->send(any.back(), shared_from_this(), message);
      any.pop_back();
    }
  }
}  // namespace kagome::network
