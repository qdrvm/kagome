/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/protocols/grandpa_protocol.hpp"

#include <libp2p/connection/loopback_stream.hpp>

#include "blockchain/block_tree.hpp"
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
      const primitives::BlockHash &genesis_hash,
      std::shared_ptr<libp2p::basic::Scheduler> scheduler)
      : base_(host,
              {fmt::format(kGrandpaProtocol, hex_lower(genesis_hash)),
               kGrandpaProtocolLegacy},
              "GrandpaProtocol"),
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

  bool GrandpaProtocol::stop() {
    return base_.stop();
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
            auto own_peer_state =
                self->peer_manager_->getPeerState(self->own_info_.id);
            if (own_peer_state.has_value()) {
              GrandpaNeighborMessage msg{
                  .round_number =
                      own_peer_state->get().round_number.value_or(1),
                  .voter_set_id = own_peer_state->get().set_id.value_or(0),
                  .last_finalized = own_peer_state->get().last_finalized};

              SL_DEBUG(self->base_.logger(),
                       "Send initial neighbor message: grandpa round number {}",
                       msg.round_number);

              auto shared_msg =
                  KAGOME_EXTRACT_SHARED_CACHE(GrandpaProtocol, GrandpaMessage);
              (*shared_msg) = GrandpaMessage(std::move(msg));

              self->stream_engine_->send(
                  stream->remotePeerId().value(), self, std::move(shared_msg));
            }

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

    read_writer->read<GrandpaMessage>([stream = std::move(stream),
                                       wp = weak_from_this()](
                                          auto &&grandpa_message_res) mutable {
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
      auto &grandpa_message = grandpa_message_res.value();

      visit_in_place(
          grandpa_message,
          [&](const network::GrandpaVote &vote_message) {
            SL_VERBOSE(self->base_.logger(),
                       "VoteMessage has received from {}",
                       peer_id);
            self->grandpa_observer_->onVoteMessage(peer_id, vote_message);
          },
          [&](const FullCommitMessage &commit_message) {
            SL_VERBOSE(self->base_.logger(),
                       "CommitMessage has received from {}",
                       peer_id);
            self->grandpa_observer_->onCommitMessage(peer_id, commit_message);
          },
          [&](const GrandpaNeighborMessage &neighbor_message) {
            if (peer_id != self->own_info_.id)
              SL_VERBOSE(self->base_.logger(),
                         "NeighborMessage has received from {}",
                         peer_id);
            self->grandpa_observer_->onNeighborMessage(peer_id,
                                                       neighbor_message);
          },
          [&](const network::CatchUpRequest &catch_up_request) {
            SL_VERBOSE(self->base_.logger(),
                       "CatchUpRequest has received from {}",
                       peer_id);
            self->grandpa_observer_->onCatchUpRequest(peer_id,
                                                      catch_up_request);
          },
          [&](const network::CatchUpResponse &catch_up_response) {
            SL_VERBOSE(self->base_.logger(),
                       "CatchUpResponse has received from {}",
                       peer_id);
            self->grandpa_observer_->onCatchUpResponse(peer_id,
                                                       catch_up_response);
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

    auto filter = [&, &msg = vote_message](const PeerId &peer_id) {
      auto info_opt = peer_manager_->getPeerState(peer_id);
      if (not info_opt.has_value()) {
        SL_DEBUG(base_.logger(),
                 "Vote signed by {} with set_id={} in round={} "
                 "has not been sent to {}: peer is not connected",
                 msg.id(),
                 msg.counter,
                 msg.round_number,
                 peer_id);
        return false;
      }
      const auto &info = info_opt.value();

      if (not info.get().set_id.has_value()
          or not info.get().round_number.has_value()) {
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
      if (msg.counter != info.get().set_id) {
        SL_DEBUG(base_.logger(),
                 "Vote signed by {} with set_id={} in round={} "
                 "has not been sent to {} as impolite: their set id is {}",
                 msg.id(),
                 msg.counter,
                 msg.round_number,
                 peer_id,
                 info.get().set_id.value());
        return false;
      }

      // If a peer is at round r, is impolite to send messages about r-2 or
      // earlier
      if (msg.round_number + 2 < info.get().round_number.value()) {
        SL_DEBUG(
            base_.logger(),
            "Vote signed by {} with set_id={} in round={} "
            "has not been sent to {} as impolite: their round is already {}",
            msg.id(),
            msg.counter,
            msg.round_number,
            peer_id,
            info.get().round_number.value());
        return false;
      }

      // If a peer is at round r, is extremely impolite to send messages about
      // r+1 or later
      if (msg.round_number > info.get().round_number.value()) {
        SL_DEBUG(base_.logger(),
                 "Vote signed by {} with set_id={} in round={} "
                 "has not been sent to {} as impolite: their round is old: {}",
                 msg.id(),
                 msg.counter,
                 msg.round_number,
                 peer_id,
                 info.get().round_number.value());
        return false;
      }

      return true;
    };

    auto shared_msg =
        KAGOME_EXTRACT_SHARED_CACHE(GrandpaProtocol, GrandpaMessage);
    (*shared_msg) = GrandpaMessage(std::move(vote_message));

    if (not peer_id.has_value()) {
      stream_engine_->broadcast<GrandpaMessage>(
          shared_from_this(), std::move(shared_msg), filter);
    } else {
      stream_engine_->send(
          peer_id.value(), shared_from_this(), std::move(shared_msg));
    };
  }

  void GrandpaProtocol::neighbor(GrandpaNeighborMessage &&msg) {
    SL_DEBUG(base_.logger(),
             "Send neighbor message: grandpa round number {}",
             msg.round_number);

    peer_manager_->updatePeerState(own_info_.id, msg);

    auto shared_msg =
        KAGOME_EXTRACT_SHARED_CACHE(GrandpaProtocol, GrandpaMessage);
    (*shared_msg) = GrandpaMessage(msg);

    stream_engine_->broadcast<GrandpaMessage>(
        shared_from_this(),
        shared_msg,
        StreamEngine::RandomGossipStrategy{
            stream_engine_->outgoingStreamsNumber(shared_from_this()),
            app_config_.luckyPeers()});
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
                   finalizing =
                       msg.message.target_number](const PeerId &peer_id) {
      auto info_opt = peer_manager_->getPeerState(peer_id);
      if (not info_opt.has_value()) {
        SL_DEBUG(base_.logger(),
                 "Commit with set_id={} in round={} "
                 "has not been sent to {}: peer is not connected",
                 set_id,
                 round_number,
                 peer_id);
        return false;
      }
      const auto &info = info_opt.value();

      if (not info.get().set_id.has_value()
          or not info.get().round_number.has_value()) {
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
      if (set_id != info.get().set_id) {
        SL_DEBUG(base_.logger(),
                 "Commit with set_id={} in round={} "
                 "has not been sent to {} as impolite: their set id is {}",
                 set_id,
                 round_number,
                 peer_id,
                 info.get().set_id.value());
        return false;
      }

      // Don't send commit if that has not actual for remote peer already
      if (round_number < info.get().round_number.value()) {
        SL_DEBUG(
            base_.logger(),
            "Commit with set_id={} in round={} "
            "has not been sent to {} as impolite: their round is already {}",
            set_id,
            round_number,
            peer_id,
            info.get().round_number.value());
        return false;
      }

      // It is impolite to send commits which are earlier than the last commit
      // sent.
      if (finalizing < info.get().last_finalized) {
        SL_DEBUG(
            base_.logger(),
            "Commit with set_id={} in round={} "
            "has not been sent to {} as impolite: their round is already {}",
            set_id,
            round_number,
            peer_id,
            info.get().round_number.value());
        return false;
      }

      return true;
    };

    auto shared_msg =
        KAGOME_EXTRACT_SHARED_CACHE(GrandpaProtocol, GrandpaMessage);
    (*shared_msg) = GrandpaMessage(std::move(msg));

    if (not peer_id.has_value()) {
      stream_engine_->broadcast<GrandpaMessage>(
          shared_from_this(), std::move(shared_msg), filter);
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
    const auto &info = info_opt.value();

    if (not info.get().set_id.has_value()
        or not info.get().round_number.has_value()) {
      SL_DEBUG(base_.logger(),
               "Catch-up-request with set_id={} in round={} "
               "has not been sent to {}: set id or round number unknown",
               catch_up_request.voter_set_id,
               catch_up_request.round_number,
               peer_id);
      return;
    }

    // Impolite to send a catch up request to a peer in a new different Set ID.
    if (catch_up_request.voter_set_id != info.get().set_id) {
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
    if (catch_up_request.round_number < info.get().round_number.value() - 1) {
      SL_DEBUG(base_.logger(),
               "Catch-up-request with set_id={} in round={} "
               "has not been sent to {}: too old round for requested",
               catch_up_request.voter_set_id,
               catch_up_request.round_number,
               peer_id);
      return;
    }

    auto round_id =
        std::tuple(info.get().round_number.value(), info.get().set_id.value());

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
    const auto &info = info_opt.value();

    if (not info.get().set_id.has_value()
        or not info.get().round_number.has_value()) {
      SL_DEBUG(base_.logger(),
               "Catch-up-response with set_id={} in round={} "
               "has not been sent to {}: set id or round number unknown",
               catch_up_response.voter_set_id,
               catch_up_response.round_number,
               peer_id);
      return;
    }

    /// Impolite to send a catch up request to a peer in a new different Set ID.
    if (catch_up_response.voter_set_id != info.get().set_id) {
      SL_DEBUG(base_.logger(),
               "Catch-up-response with set_id={} in round={} "
               "has not been sent to {}: {} set id",
               catch_up_response.voter_set_id,
               catch_up_response.round_number,
               peer_id,
               info.get().set_id.has_value() ? "different" : "unknown");
      return;
    }

    /// Avoid sending useless response (if peer is already caught up)
    if (catch_up_response.round_number < info.get().round_number) {
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

}  // namespace kagome::network
