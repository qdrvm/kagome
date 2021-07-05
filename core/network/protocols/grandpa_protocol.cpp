/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/protocols/grandpa_protocol.hpp"

#include <libp2p/connection/loopback_stream.hpp>

#include "network/common.hpp"
#include "network/protocols/protocol_error.hpp"
#include "network/types/grandpa_message.hpp"
#include "network/types/roles.hpp"

namespace kagome::network {
  using libp2p::connection::LoopbackStream;

  GrandpaProtocol::GrandpaProtocol(
      libp2p::Host &host,
      std::shared_ptr<boost::asio::io_context> io_context,
      const application::AppConfiguration &app_config,
      std::shared_ptr<consensus::grandpa::GrandpaObserver> grandpa_observer,
      const OwnPeerInfo &own_info,
      std::shared_ptr<StreamEngine> stream_engine)
      : host_(host),
        io_context_(std::move(io_context)),
        app_config_(app_config),
        grandpa_observer_(std::move(grandpa_observer)),
        own_info_(own_info),
        stream_engine_(std::move(stream_engine)) {
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
          } else {
            SL_VERBOSE(self->log_,
                       "Fail establishing incoming {} stream with {}: {}",
                       self->protocol_,
                       stream->remotePeerId().value().toBase58(),
                       stream_res.error().message());
          }
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
            cb(stream_res.as_failure());
            return;
          }
          auto &stream = stream_res.value();

          self->writeHandshake(
              std::move(stream), Direction::OUTGOING, std::move(cb));
        });
  }

  void GrandpaProtocol::readHandshake(
      std::shared_ptr<Stream> stream,
      Direction direction,
      std::function<void(outcome::result<std::shared_ptr<Stream>>)> &&cb) {
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
            self->log_->error("Error while reading roles: {}",
                              remote_roles_res.error().message());
            stream->reset();
            cb(remote_roles_res.as_failure());
            return;
          }
          auto &remote_roles = remote_roles_res.value();

          switch (direction) {
            case Direction::OUTGOING:
              std::ignore = self->stream_engine_->addOutgoing(stream, self);
              self->read(stream);
              cb(stream);
              break;
            case Direction::INCOMING:
              self->writeHandshake(std::move(stream), direction, std::move(cb));
              break;
          }
        });
  }

  void GrandpaProtocol::writeHandshake(
      std::shared_ptr<Stream> stream,
      Direction direction,
      std::function<void(outcome::result<std::shared_ptr<Stream>>)> &&cb) {
    auto read_writer = std::make_shared<ScaleMessageReadWriter>(stream);

    Roles roles = app_config_.roles();

    read_writer->write(
        roles,
        [stream, direction, wp = weak_from_this(), cb = std::move(cb)](
            auto &&write_res) mutable {
          auto self = wp.lock();
          if (not self) {
            stream->reset();
            cb(ProtocolError::GONE);
            return;
          }

          if (not write_res.has_value()) {
            self->log_->error("Error while writing own roles: {}",
                              write_res.error().message());
            stream->reset();
            cb(write_res.as_failure());
            return;
          }

          switch (direction) {
            case Direction::OUTGOING:
              self->readHandshake(std::move(stream), direction, std::move(cb));
              break;
            case Direction::INCOMING:
              self->read(std::move(stream));
              cb(stream);
              break;
          }
        });
  }

  void GrandpaProtocol::read(std::shared_ptr<Stream> stream) {
    auto read_writer = std::make_shared<ScaleMessageReadWriter>(stream);

    read_writer->read<GrandpaMessage>(
        [stream, wp = weak_from_this()](auto &&grandpa_message_res) mutable {
          auto self = wp.lock();
          if (not self) {
            stream->reset();
            return;
          }

          if (not grandpa_message_res) {
            self->log_->error("Error while reading grandpa message: {}",
                              grandpa_message_res.error().message());
            stream->reset();
            return;
          }

          auto peer_id = stream->remotePeerId().value();
          auto &grandpa_message = grandpa_message_res.value();

          visit_in_place(
              grandpa_message,
              [&](const network::GrandpaVote &vote_message) {
                self->grandpa_observer_->onVoteMessage(peer_id, vote_message);
              },
              [&](const network::GrandpaCommit &fin_message) {
                self->grandpa_observer_->onFinalize(peer_id, fin_message);
              },
              [&](const GrandpaNeighborMessage &neighbor_message) {
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
              },
              [&](const auto &...) {
                BOOST_ASSERT_MSG(false, "Unknown variant of grandpa message");
                stream->reset();
              });
          self->read(std::move(stream));
        });
  }

  void GrandpaProtocol::write(
      std::shared_ptr<Stream> stream,
      const int &message,
      std::function<void(outcome::result<std::shared_ptr<Stream>>)> &&cb) {
    auto read_writer = std::make_shared<ScaleMessageReadWriter>(stream);

    // NOTE: It will changed as soon as this protocol will be implemented in
    // according with spec if any
    throw "It is not implemented yet";
  }

}  // namespace kagome::network
