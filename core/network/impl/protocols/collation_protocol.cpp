/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/protocols/collation_protocol.hpp"

#include <iostream>

#include "network/common.hpp"
#include "network/helpers/peer_id_formatter.hpp"
#include "network/helpers/scale_message_read_writer.hpp"
#include "network/impl/protocols/protocol_error.hpp"

namespace kagome::network {

  CollationProtocol::CollationProtocol(
      libp2p::Host &host,
      application::AppConfiguration const &app_config,
      application::ChainSpec const & /*chain_spec*/,
      std::shared_ptr<CollationObserver> observer)
      : base_(host, {kCollationProtocol}, "CollationProtocol"),
        observer_(std::move(observer)),
        app_config_{app_config} {}

  bool CollationProtocol::start() {
    return base_.start(weak_from_this());
  }

  bool CollationProtocol::stop() {
    return base_.stop();
  }

  void CollationProtocol::newOutgoingStream(
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
                       stream_res.error().message());
            cb(stream_res.as_failure());
            return;
          }

          auto &stream = stream_res.value().stream;
          BOOST_ASSERT(stream->remotePeerId().has_value());
          self->doCollatorHandshake<false>(
              stream,
              [wptr{wp},
               cb{std::move(cb)}](std::shared_ptr<Stream> const &stream) {
                if (!stream)
                  cb(ProtocolError::HANDSHAKE_ERROR);
                else
                  cb(stream);
              });
        });
  }

  void CollationProtocol::onCollationDeclRx(
      libp2p::peer::PeerId const &peer_id,
      CollatorDeclaration &&collation_decl) {
    if (observer_) {
      observer_->onDeclare(peer_id,
                           collation_decl.collator_id,
                           collation_decl.para_id,
                           collation_decl.signature);
    }
  }

  void CollationProtocol::onCollationAdvRx(
      libp2p::peer::PeerId const &peer_id,
      CollatorAdvertisement &&collation_adv) {
    if (observer_) {
      observer_->onAdvertise(peer_id, collation_adv.relay_parent);
    }
  }

  void CollationProtocol::onCollationMessageRx(
      libp2p::peer::PeerId const &peer_id,
      CollationMessage &&collation_message) {
    visit_in_place(
        std::move(collation_message),
        [&](network::CollatorDeclaration &&collation_decl) {
          onCollationDeclRx(peer_id, std::move(collation_decl));
        },
        [&](network::CollatorAdvertisement &&collation_adv) {
          onCollationAdvRx(peer_id, std::move(collation_adv));
        },
        [&](auto &&) {
          SL_WARN(base_.logger(), "Unexpected collation message from.");
        });
  }

  void CollationProtocol::readCollationMsg(
      std::shared_ptr<kagome::network::Stream> stream) {
    auto read_writer = std::make_shared<ScaleMessageReadWriter>(stream);
    read_writer->template read<WireMessage>(
        [wptr{weak_from_this()},
         stream{std::move(stream)}](auto &&result) mutable {
          auto self = wptr.lock();
          if (!self) {
            stream->close([](auto &&) {});
            return;
          }

          if (!result) {
            SL_WARN(self->base_.logger(),
                    "Can't read incoming collation message from stream {} with "
                    "error {}",
                    stream->remotePeerId().value(),
                    result.error().message());
            self->base_.closeStream(wptr, stream);
            return;
          }

          SL_VERBOSE(self->base_.logger(),
                     "Received collation message from {}",
                     stream->remotePeerId().has_value()
                         ? stream->remotePeerId().value().toBase58()
                         : "{no peerId}");

          visit_in_place(
              std::move(result.value()),
              [&](ViewUpdate &&) {
                SL_VERBOSE(self->base_.logger(),
                           "Received ViewUpdate from {}",
                           stream->remotePeerId().has_value()
                               ? stream->remotePeerId().value().toBase58()
                               : "{no peerId}");
              },
              [&](ProtocolMessage &&p) {
                visit_in_place(std::move(p), [&](CollationMessage &&m) {
                  BOOST_ASSERT(stream->remotePeerId().has_value());
                  self->onCollationMessageRx(stream->remotePeerId().value(),
                                             std::move(m));
                });
              });
          self->readCollationMsg(std::move(stream));
        });
  }

  void CollationProtocol::onIncomingStream(std::shared_ptr<Stream> stream) {
    BOOST_ASSERT(stream->remotePeerId().has_value());
    doCollatorHandshake<true>(
        stream,
        [wptr{weak_from_this()}](
            std::shared_ptr<Stream> const &stream) mutable {
          if (!stream) {
            return;
          }
          if (auto self = wptr.lock()) {
            self->readCollationMsg(stream);
          }
        });
  }

}  // namespace kagome::network
