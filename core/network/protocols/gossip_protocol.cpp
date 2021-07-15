/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/protocols/gossip_protocol.hpp"

#include <libp2p/connection/loopback_stream.hpp>

#include "network/common.hpp"
#include "network/protocols/protocol_error.hpp"

namespace kagome::network {
  using libp2p::connection::LoopbackStream;

  GossipProtocol::GossipProtocol(
      libp2p::Host &host,
      std::shared_ptr<boost::asio::io_context> io_context,
      std::shared_ptr<consensus::grandpa::GrandpaObserver> grandpa_observer,
      const OwnPeerInfo &own_info,
      std::shared_ptr<StreamEngine> stream_engine)
      : host_(host),
        io_context_(std::move(io_context)),
        grandpa_observer_(std::move(grandpa_observer)),
        own_info_(own_info),
        stream_engine_(std::move(stream_engine)) {
    BOOST_ASSERT(io_context_ != nullptr);
    BOOST_ASSERT(grandpa_observer_ != nullptr);
    BOOST_ASSERT(stream_engine_ != nullptr);
    const_cast<Protocol &>(protocol_) = kGossipProtocol;
  }

  bool GossipProtocol::start() {
    auto stream = std::make_shared<LoopbackStream>(own_info_, io_context_);
    auto res = stream_engine_->add(stream, shared_from_this());
    if (not res.has_value()) {
      return false;
    }
    readGossipMessage(std::move(stream));

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

  bool GossipProtocol::stop() {
    return true;
  }

  void GossipProtocol::onIncomingStream(std::shared_ptr<Stream> stream) {
    BOOST_ASSERT(stream->remotePeerId().has_value());

    auto res = stream_engine_->addIncoming(stream, shared_from_this());
    if (res.has_value()) {
      SL_VERBOSE(log_,
                 "Fully established incoming {} stream with {}",
                 protocol_,
                 stream->remotePeerId().value().toBase58());
      readGossipMessage(std::move(stream));
      return;
    }

    SL_VERBOSE(log_,
               "Fail at adding to engine incoming {} stream with {}: {}",
               protocol_,
               stream->remotePeerId().value().toBase58(),
               res.error().message());
    stream->reset();
  }

  void GossipProtocol::newOutgoingStream(
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

          auto res = self->stream_engine_->addOutgoing(stream, self);
          if (res.has_value()) {
            cb(stream);
            self->readGossipMessage(std::move(stream));
          } else {
            cb(res.as_failure());
          }
        });
  }

  void GossipProtocol::readGossipMessage(std::shared_ptr<Stream> stream) {
    auto read_writer = std::make_shared<ScaleMessageReadWriter>(stream);

    read_writer->read<GossipMessage>([stream, wp = weak_from_this()](
                                         auto &&gossip_message_res) mutable {
      auto self = wp.lock();
      if (not self) {
        stream->reset();
        return;
      }

      if (not gossip_message_res) {
        SL_VERBOSE(self->log_,
                   "Error while reading gossip message: {}",
                   gossip_message_res.error().message());
        stream->reset();
        return;
      }

      auto peer_id = stream->remotePeerId().value();
      auto &gossip_message = gossip_message_res.value();

      using MsgType = GossipMessage::Type;

      bool success;

      switch (gossip_message.type) {
        case MsgType::BLOCK_ANNOUNCE: {
          BOOST_ASSERT(!"Legacy protocol unsupported!");
          self->log_->warn("Legacy protocol message BLOCK_ANNOUNCE from: {}",
                           peer_id.toBase58());
          success = false;
          break;
        }
        case MsgType::TRANSACTIONS: {
          BOOST_ASSERT(!"Legacy protocol unsupported!");
          self->log_->warn("Legacy protocol message TRANSACTIONS from: {}",
                           peer_id.toBase58());
          success = false;
          break;
        }
        case GossipMessage::Type::STATUS: {
          self->log_->error("Status message processing is not implemented yet");
          success = false;
          break;
        }
        case GossipMessage::Type::BLOCK_REQUEST: {
          self->log_->error(
              "BlockRequest message processing is not implemented yet");
          success = false;
          break;
        }
        case GossipMessage::Type::UNKNOWN:
        default: {
          self->log_->error("unknown message type is set");
          success = false;
          break;
        }
      }

      if (success) {
        self->readGossipMessage(std::move(stream));
      } else {
        stream->reset();
      }
    });
  }

  //  void GossipProtocol::writeGossipMessage(std::shared_ptr<Stream> stream,
  //                                          const GossipMessage
  //                                          &gossip_message) {
  //    std::function<void(outcome::result<std::shared_ptr<Stream>>)> cb =
  //        [](outcome::result<std::shared_ptr<Stream>>) {};
  //
  //    auto read_writer = std::make_shared<ScaleMessageReadWriter>(stream);
  //
  //    read_writer->write(gossip_message,
  //                       [stream, wp = weak_from_this(), cb = std::move(cb)](
  //                           auto &&write_res) mutable {
  //                         auto self = wp.lock();
  //                         if (not self) {
  //                           stream->reset();
  //                           cb(ProtocolError::GONE);
  //                           return;
  //                         }
  //
  //                         if (not write_res.has_value()) {
  //                           self->log_->error(
  //                               "Error while writing block announce: {}",
  //                               write_res.error().message());
  //                           stream->reset();
  //                           cb(write_res.as_failure());
  //                           return;
  //                         }
  //
  //                         cb(std::move(stream));
  //                       });
  //  }

}  // namespace kagome::network
