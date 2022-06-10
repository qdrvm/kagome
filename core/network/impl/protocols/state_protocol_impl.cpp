/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/protocols/state_protocol_impl.hpp"
#include "network/adapters/protobuf_state_request.hpp"
#include "network/adapters/protobuf_state_response.hpp"
#include "network/common.hpp"
#include "network/helpers/peer_id_formatter.hpp"
#include "network/helpers/protobuf_message_read_writer.hpp"
#include "network/impl/protocols/protocol_error.hpp"

namespace kagome::network {

  StateProtocolImpl::StateProtocolImpl(libp2p::Host &host,
                                       const application::ChainSpec &chain_spec)
      : host_(host) {
    const_cast<Protocol &>(protocol_) =
        fmt::format(kStateProtocol.data(), chain_spec.protocolId());
  }

  bool StateProtocolImpl::start() {
    host_.setProtocolHandler(protocol_, [wp = weak_from_this()](auto &&stream) {
      if (auto self = wp.lock()) {
        if (auto peer_id = stream->remotePeerId()) {
          SL_TRACE(self->log_,
                   "Handled {} protocol stream from {:l}",
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

  bool StateProtocolImpl::stop() {
    return true;
  }

  void StateProtocolImpl::onIncomingStream(std::shared_ptr<Stream> stream) {
    BOOST_ASSERT(stream->remotePeerId().has_value());
  }

  void StateProtocolImpl::newOutgoingStream(
      const PeerInfo &peer_info,
      std::function<void(outcome::result<std::shared_ptr<Stream>>)> &&cb) {
    SL_DEBUG(log_, "Connect for {} stream with {}", protocol_, peer_info.id);

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
            SL_VERBOSE(
                self->log_,
                "Error happened while connection over {} stream with {}: {}",
                self->protocol_,
                peer_id,
                stream_res.error().message());
            cb(stream_res.as_failure());
            return;
          }
          auto &stream = stream_res.value();

          SL_DEBUG(self->log_,
                   "Established connection over {} stream with {}",
                   self->protocol_,
                   peer_id);

          cb(std::move(stream));
        });
  }

  void StateProtocolImpl::request(
      const PeerId &peer_id,
      StateRequest state_request,
      std::function<void(outcome::result<StateResponse>)> &&response_handler) {
    auto addresses_res =
        host_.getPeerRepository().getAddressRepository().getAddresses(peer_id);
    if (not addresses_res.has_value()) {
      response_handler(addresses_res.as_failure());
      return;
    }

    newOutgoingStream(
        {peer_id, addresses_res.value()},
        [wp = weak_from_this(),
         response_handler = std::move(response_handler),
         state_request = std::move(state_request)](auto &&stream_res) mutable {
          if (not stream_res.has_value()) {
            response_handler(stream_res.as_failure());
            return;
          }
          auto &stream = stream_res.value();

          auto self = wp.lock();
          if (not self) {
            stream->reset();
            response_handler(ProtocolError::GONE);
            return;
          }

          SL_WARN(self->log_,
                  "Established outgoing {} stream with {}",
                  self->protocol_,
                  stream->remotePeerId().value());

          self->writeRequest(stream,
                             std::move(state_request),
                             [stream,
                              wp = std::move(wp),
                              response_handler = std::move(response_handler)](
                                 auto &&write_res) mutable {
                               auto self = wp.lock();
                               if (not self) {
                                 stream->reset();
                                 response_handler(ProtocolError::GONE);
                                 return;
                               }

                               SL_WARN(self->log_, "State request sent");

                               if (not write_res.has_value()) {
                                 SL_WARN(self->log_, "Error getting response");
                                 stream->reset();
                                 response_handler(write_res.as_failure());
                                 return;
                               }

                               self->readResponse(std::move(stream),
                                                  std::move(response_handler));
                             });
        });
  }

  void StateProtocolImpl::writeRequest(
      std::shared_ptr<Stream> stream,
      StateRequest state_request,
      std::function<void(outcome::result<void>)> &&cb) {
    auto read_writer = std::make_shared<ProtobufMessageReadWriter>(stream);

    SL_DEBUG(log_,
             "Write request info outgoing {} stream with {}",
             protocol_,
             stream->remotePeerId().value());

    read_writer->write(
        state_request,
        [stream, wp = weak_from_this(), cb = std::move(cb)](
            auto &&write_res) mutable {
          auto self = wp.lock();
          if (not self) {
            stream->reset();
            cb(ProtocolError::GONE);
            return;
          }

          if (not write_res.has_value()) {
            SL_VERBOSE(
                self->log_,
                "Error at write request into outgoing {} stream with {}: {}",
                self->protocol_,
                stream->remotePeerId().value(),
                write_res.error().message());

            stream->reset();
            cb(write_res.as_failure());
            return;
          }

          SL_DEBUG(self->log_,
                   "Request written successful into outgoing {} stream with {}",
                   self->protocol_,
                   stream->remotePeerId().value());

          cb(outcome::success());
        });
  }

  void StateProtocolImpl::readResponse(
      std::shared_ptr<Stream> stream,
      std::function<void(outcome::result<StateResponse>)> &&response_handler) {
    auto read_writer = std::make_shared<ProtobufMessageReadWriter>(stream);

    SL_WARN(log_,
            "Read response from outgoing {} stream with {}",
            protocol_,
            stream->remotePeerId().value());

    read_writer->read<StateResponse>([stream,
                                      wp = weak_from_this(),
                                      response_handler =
                                          std::move(response_handler)](
                                         auto &&state_response_res) mutable {
      auto self = wp.lock();
      if (not self) {
        stream->reset();
        response_handler(ProtocolError::GONE);
        return;
      }

      if (not state_response_res.has_value()) {
        SL_WARN(self->log_,
                "Error at read response from outgoing {} stream with {}: {}",
                self->protocol_,
                stream->remotePeerId().value(),
                state_response_res.error().message());

        stream->reset();
        response_handler(state_response_res.as_failure());
        return;
      }
      auto &state_response = state_response_res.value();

      SL_DEBUG(self->log_,
               "Successful response read from outgoing {} stream with {}",
               self->protocol_,
               stream->remotePeerId().value());

      stream->reset();
      response_handler(std::move(state_response));
    });
  }

}  // namespace kagome::network
