/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/protocols/grandpa_protocol.hpp"

#include <libp2p/connection/loopback_stream.hpp>

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
      std::shared_ptr<PeerManager> peer_manager)
      : host_(host),
        io_context_(std::move(io_context)),
        app_config_(app_config),
        grandpa_observer_(std::move(grandpa_observer)),
        own_info_(own_info),
        stream_engine_(std::move(stream_engine)),
        peer_manager_(std::move(peer_manager)) {
    const_cast<Protocol &>(protocol_) = kGrandpaProtocol;
  }

  bool GrandpaProtocol::start() {
    auto stream = std::make_shared<LoopbackStream>(own_info_, io_context_);
    auto res = stream_engine_->add(stream, shared_from_this());
    if (not res.has_value()) {
      return false;
    }
    read(std::move(stream));

    host_.setProtocolHandler(protocol_, [wp = weak_from_this()](auto &&stream) {
      if (auto self = wp.lock()) {
        if (auto peer_id = stream->remotePeerId()) {
          SL_TRACE(self->log_,
                   "Handled {} protocol stream from: {}",
                   self->protocol_,
                   peer_id.value());
          self->onIncomingStream(std::forward<decltype(stream)>(stream));
          return;
        }
        self->log_->warn("Handled {} protocol stream from unknown peer",
                         self->protocol_);
      }
    });
    return true;
  }

  bool GrandpaProtocol::stop() {
    return true;
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
            SL_VERBOSE(self->log_,
                       "Handshake failed on incoming {} stream with {}: {}",
                       self->protocol_,
                       peer_id,
                       res.error().message());
            stream->reset();
            return;
          }

          res = self->stream_engine_->addIncoming(stream, self);
          if (not res.has_value()) {
            SL_VERBOSE(self->log_,
                       "Can't register incoming {} stream with {}: {}",
                       self->protocol_,
                       peer_id,
                       res.error().message());
            stream->reset();
            return;
          }

          SL_VERBOSE(self->log_,
                     "Fully established incoming {} stream with {}",
                     self->protocol_,
                     peer_id);
        });
  }

  void GrandpaProtocol::newOutgoingStream(
      const PeerInfo &peer_info,
      std::function<void(outcome::result<std::shared_ptr<Stream>>)> &&cb) {
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

            // Send must first neighbor message
            auto own_peer_state =
                self->peer_manager_->getPeerState(self->own_info_.id);
            if (own_peer_state.has_value()) {
              self->neighbor(GrandpaNeighborMessage{
                  .round_number = own_peer_state->round_number,
                  .voter_set_id = own_peer_state->set_id,
                  .last_finalized = own_peer_state->last_finalized});
            }

            cb(std::move(stream));
          };

          self->writeHandshake(
              std::move(stream), Direction::OUTGOING, std::move(cb2));
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
            SL_VERBOSE(self->log_,
                       "Can't read handshake from {}: {}",
                       stream->remotePeerId().value(),
                       remote_roles_res.error().message());
            stream->reset();
            cb(remote_roles_res.as_failure());
            return;
          }
          // auto &remote_roles = remote_roles_res.value();

          SL_TRACE(self->log_,
                   "Handshake has received from {}",
                   stream->remotePeerId().value());

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
        SL_VERBOSE(self->log_,
                   "Can't read grandpa message from {}: {}",
                   stream->remotePeerId().value(),
                   grandpa_message_res.error().message());
        stream->reset();
        return;
      }

      auto peer_id = stream->remotePeerId().value();
      auto &grandpa_message = grandpa_message_res.value();

      SL_VERBOSE(self->log_, "Message has received from {}", peer_id);

      visit_in_place(
          grandpa_message,
          [&](const network::GrandpaVote &vote_message) {
            self->grandpa_observer_->onVoteMessage(peer_id, vote_message);
          },
          [&](const FullCommitMessage &commit_message) {
            self->grandpa_observer_->onCommitMessage(peer_id, commit_message);
          },
          [&](const GrandpaNeighborMessage &neighbor_message) {
            self->peer_manager_->updatePeerState(peer_id, neighbor_message);
            self->grandpa_observer_->onNeighborMessage(peer_id,
                                                       neighbor_message);
          },
          [&](const network::CatchUpRequest &catch_up_request) {
            self->grandpa_observer_->onCatchUpRequest(peer_id,
                                                      catch_up_request);
          },
          [&](const network::CatchUpResponse &catch_up_response) {
            self->grandpa_observer_->onCatchUpResponse(peer_id,
                                                       catch_up_response);
          });
      self->read(std::move(stream));
    });
  }

  void GrandpaProtocol::vote(network::GrandpaVote &&vote_message) {
    SL_DEBUG(log_,
             "Send vote message: grandpa round number {}",
             vote_message.round_number);

    auto filter = [this,
                   msg = vote_message,
                   set_id = vote_message.counter,
                   round_number =
                       vote_message.round_number](const PeerId &peer_id) {
      auto info_opt = peer_manager_->getPeerState(peer_id);
      if (not info_opt.has_value()) {
        SL_DEBUG(
            log_,
            "Vote signed by {} with set_id={} in round={} has not sent to {}: "
            "peen is not connected",
            msg.id(),
            msg.counter,
            msg.round_number,
            peer_id);
        return false;
      }
      const auto &info = info_opt.value();

      // If a peer is at a given voter set, it is impolite to send messages
      // from an earlier voter set. It is extremely impolite to send messages
      // from a future voter set.
      if (set_id != info.set_id) {
        SL_DEBUG(
            log_,
            "Vote signed by {} with set_id={} in round={} has not sent to {} "
            "as impolite: their set id is {}",
            msg.id(),
            msg.counter,
            msg.round_number,
            peer_id,
            info.set_id);
        return false;
      }

      // If a peer is at round r, is impolite to send messages about r-2 or
      // earlier
      if (round_number + 2 < info.round_number) {
        SL_DEBUG(
            log_,
            "Vote signed by {} with set_id={} in round={} has not sent to {} "
            "as impolite: their round is already {}",
            msg.id(),
            msg.counter,
            msg.round_number,
            peer_id,
            info.round_number);
        return false;
      }

      // If a peer is at round r, is extremely impolite to send messages about
      // r+1 or later
      if (round_number > info.round_number) {
        SL_DEBUG(
            log_,
            "Vote signed by {} with set_id={} in round={} has not sent to {} "
            "as impolite: their round too old: {}",
            msg.id(),
            msg.counter,
            msg.round_number,
            peer_id,
            info.round_number);
        return false;
      }

      return true;
    };

    auto shared_msg =
        KAGOME_EXTRACT_SHARED_CACHE(GrandpaProtocol, GrandpaMessage);
    (*shared_msg) = GrandpaMessage(std::move(vote_message));

    stream_engine_->broadcast<GrandpaMessage>(
        shared_from_this(), std::move(shared_msg), filter);
  }

  void GrandpaProtocol::neighbor(GrandpaNeighborMessage &&msg) {
    SL_DEBUG(log_,
             "Send neighbor message: grandpa round number {}",
             msg.round_number);

    peer_manager_->updatePeerState(own_info_.id, msg);

    auto shared_msg =
        KAGOME_EXTRACT_SHARED_CACHE(GrandpaProtocol, GrandpaMessage);
    (*shared_msg) = GrandpaMessage(std::move(msg));

    stream_engine_->broadcast<GrandpaMessage>(shared_from_this(),
                                              std::move(shared_msg));
  }

  void GrandpaProtocol::finalize(FullCommitMessage &&msg) {
    SL_DEBUG(log_, "Send commit message: grandpa round number {}", msg.round);

    auto filter = [this,
                   set_id = msg.set_id,
                   round_number = msg.round,
                   finalizing =
                       msg.message.target_number](const PeerId &peer_id) {
      auto info_opt = peer_manager_->getPeerState(peer_id);
      if (not info_opt.has_value()) {
        SL_DEBUG(
            log_,
            "Commit with set_id={} in round={} has not sent to {}: "
            "peen is not connected",
            set_id,
            round_number,
            peer_id);
        return false;
      }
      const auto &info = info_opt.value();

      // It is especially impolite to send commits which are invalid, or from
      // a different Set ID than the receiving peer has indicated.
      if (set_id != info.set_id) {
        SL_DEBUG(
            log_,
            "Commit with set_id={} in round={} has not sent to {} as impolite: "
            "their set id is {}",
            set_id,
            round_number,
            peer_id,
            info.set_id);
        return false;
      }

      // If a peer is at round r, is impolite to send messages about r-2 or
      // earlier and extremely impolite to send messages about r+1 or later.
      if (round_number < info.round_number) {
        SL_DEBUG(
            log_,
            "Commit with set_id={} in round={} has not sent to {} as impolite: "
            "their round is already {}",
            set_id,
            round_number,
            peer_id,
            info.round_number);
        return false;
      }

      // It is impolite to send commits which are earlier than the last commit
      // sent.
      if (finalizing < info.last_finalized) {
        SL_DEBUG(
            log_,
            "Commit with set_id={} in round={} has not sent to {} as impolite: "
            "their round is already {}",
            set_id,
            round_number,
            peer_id,
            info.round_number);
        return false;
      }

      return true;
    };

    auto shared_msg =
        KAGOME_EXTRACT_SHARED_CACHE(GrandpaProtocol, GrandpaMessage);
    (*shared_msg) = GrandpaMessage(std::move(msg));

    stream_engine_->broadcast<GrandpaMessage>(
        shared_from_this(), std::move(shared_msg), filter);
  }

  void GrandpaProtocol::catchUpRequest(const libp2p::peer::PeerId &peer_id,
                                       CatchUpRequest &&catch_up_request) {
    SL_DEBUG(log_,
             "Send catch-up request: beginning with grandpa round number {}",
             catch_up_request.round_number);

    auto info_opt = peer_manager_->getPeerState(peer_id);
    if (not info_opt.has_value()) {
      SL_DEBUG(
          log_,
          "Catch-up-request with set_id={} in round={} has not sent to {}: "
          "peen is not connected",
          catch_up_request.voter_set_id,
          catch_up_request.round_number,
          peer_id);
      return;
    }
    const auto &info = info_opt.value();

    /// Impolite to send a catch up request to a peer in a new different Set ID.
    if (catch_up_request.voter_set_id != info.set_id) {
      SL_DEBUG(
          log_,
          "Catch-up-request with set_id={} in round={} has not sent to {}: "
          "different set id",
          catch_up_request.voter_set_id,
          catch_up_request.round_number,
          peer_id);
      return;
    }

    /// It is impolite to send a catch up request for a round `R` to a peer
    /// whose announced view is behind `R`.
    if (catch_up_request.round_number < info.round_number - 1) {
      SL_DEBUG(
          log_,
          "Catch-up-request with set_id={} in round={} has not sent to {}: "
          "too old round for requested",
          catch_up_request.voter_set_id,
          catch_up_request.round_number,
          peer_id);
      return;
    }

    auto shared_msg =
        KAGOME_EXTRACT_SHARED_CACHE(GrandpaProtocol, GrandpaMessage);
    (*shared_msg) = GrandpaMessage(std::move(catch_up_request));

    stream_engine_->send(peer_id, shared_from_this(), std::move(shared_msg));
  }

  void GrandpaProtocol::catchUpResponse(const libp2p::peer::PeerId &peer_id,
                                        CatchUpResponse &&catch_up_response) {
    SL_DEBUG(log_,
             "Send catch-up response: beginning with grandpa round number {}",
             catch_up_response.round_number);

    auto info_opt = peer_manager_->getPeerState(peer_id);
    if (not info_opt.has_value()) {
      SL_DEBUG(
          log_,
          "Catch-up-response with set_id={} in round={} has not sent to {}: "
          "peen is not connected",
          catch_up_response.voter_set_id,
          catch_up_response.round_number,
          peer_id);
      return;
    }
    const auto &info = info_opt.value();

    /// Impolite to send a catch up request to a peer in a new different Set ID.
    if (catch_up_response.voter_set_id != info.set_id) {
      SL_DEBUG(
          log_,
          "Catch-up-response with set_id={} in round={} has not sent to {}: "
          "different set id",
          catch_up_response.voter_set_id,
          catch_up_response.round_number,
          peer_id);
      return;
    }

    /// Avoid sending useless response (if peer is already caught up)
    if (catch_up_response.round_number < info.round_number) {
      SL_DEBUG(
          log_,
          "Catch-up-response with set_id={} in round={} has not sent to {}: "
          "is already not actual",
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
