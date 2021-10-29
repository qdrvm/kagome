/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "utils/grandpa_protocol.hpp"

#include "network/common.hpp"
#include "network/impl/protocols/protocol_error.hpp"
#include "network/types/grandpa_message.hpp"
#include "network/types/roles.hpp"

namespace kagome::network {
  GrandpaProtocol::GrandpaProtocol(libp2p::Host &host,
                                   std::shared_ptr<StreamEngine> stream_engine)
      : host_(host), stream_engine_(std::move(stream_engine)) {
    const_cast<Protocol &>(protocol_) = kGrandpaProtocol;
  }

  bool GrandpaProtocol::start() {
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
                       peer_id.toBase58(),
                       res.error().message());
            stream->reset();
            return;
          }

          res = self->stream_engine_->addIncoming(stream, self);
          if (not res.has_value()) {
            SL_VERBOSE(self->log_,
                       "Can't register incoming {} stream with {}: {}",
                       self->protocol_,
                       peer_id.toBase58(),
                       res.error().message());
            stream->reset();
            return;
          }

          SL_VERBOSE(self->log_,
                     "Fully established incoming {} stream with {}",
                     self->protocol_,
                     peer_id.toBase58());
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
                       peer_id.toBase58(),
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
                         stream->remotePeerId().value().toBase58(),
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
                         stream->remotePeerId().value().toBase58(),
                         res.error().message());
              stream->reset();
              cb(res.as_failure());
              return;
            }

            SL_VERBOSE(self->log_,
                       "Fully established outgoing {} stream with {}",
                       self->protocol_,
                       stream->remotePeerId().value().toBase58());
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
                       stream->remotePeerId().value().toBase58(),
                       remote_roles_res.error().message());
            stream->reset();
            cb(remote_roles_res.as_failure());
            return;
          }
          // auto &remote_roles = remote_roles_res.value();

          SL_TRACE(self->log_,
                   "Handshake has received from {}",
                   stream->remotePeerId().value().toBase58());

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

    Roles roles;
    roles.flags.full = 1;

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
                                      stream->remotePeerId().value().toBase58(),
                                      write_res.error().message());
                           stream->reset();
                           cb(write_res.as_failure());
                           return;
                         }

                         SL_TRACE(self->log_,
                                  "Handshake has sent to {}",
                                  stream->remotePeerId().value().toBase58());

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
            SL_VERBOSE(self->log_,
                       "Can't read grandpa message from {}: {}",
                       stream->remotePeerId().value().toBase58(),
                       grandpa_message_res.error().message());
            stream->reset();
            return;
          }

          auto peer_id = stream->remotePeerId().value();
          auto &grandpa_message = grandpa_message_res.value();

          SL_VERBOSE(
              self->log_, "Message has received from {}", peer_id.toBase58());

          visit_in_place(
              grandpa_message,
              [&](const network::GrandpaVote &vote_message) {
                /*
                 * self->grandpa_observer_->onVoteMessage(peer_id,
                 * vote_message);
                 */
              },
              [&](const FullCommitMessage &fin_message) {
                /*
                 * self->grandpa_observer_->onFinalize(peer_id, fin_message);
                 */
              },
              [&](const GrandpaNeighborMessage &msg) {
                SL_DEBUG(self->log_,
                         "NeighborMessage has received from {}: "
                         "voter_set_id={} round={} last_finalized={}",
                         peer_id.toBase58(),
                         msg.voter_set_id,
                         msg.round_number,
                         msg.last_finalized);
              },
              [&](const network::CatchUpRequest &catch_up_request) {
                /*
                 * self->grandpa_observer_->onCatchUpRequest(peer_id,
                 *                                           catch_up_request);
                 */
              },
              [&](const network::CatchUpResponse &catch_up_response) {
                /*
                 * self->grandpa_observer_->onCatchUpResponse(peer_id,
                 *                                            catch_up_response);
                 */
              });
          self->read(std::move(stream));
        });
  }

}  // namespace kagome::network
