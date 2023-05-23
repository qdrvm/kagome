/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/protocols/send_dispute_protocol_impl.hpp"

#include "log/logger.hpp"
#include "network/common.hpp"
#include "network/helpers/peer_id_formatter.hpp"
#include "network/helpers/scale_message_read_writer.hpp"
#include "network/impl/protocols/protocol_error.hpp"
#include "network/types/dispute_messages.hpp"

namespace kagome::network {

  SendDisputeProtocolImpl::SendDisputeProtocolImpl(
      Host &host,
      const application::ChainSpec &chain_spec,
      const primitives::BlockHash &genesis_hash,
      std::shared_ptr<DisputeRequestObserver> dispute_observer,
      std::shared_ptr<ReputationRepository> reputation_repository)
      : base_(kDisputeProtocolName,
              host,
              make_protocols(kSendDisputeProtocol, genesis_hash, chain_spec),
              log::createLogger(kDisputeProtocolName, "dispute_protocol")),
        dispute_observer_(std::move(dispute_observer)),
        reputation_repository_(std::move(reputation_repository)) {
    BOOST_ASSERT(dispute_observer_ != nullptr);
    BOOST_ASSERT(reputation_repository_ != nullptr);
  }

  bool SendDisputeProtocolImpl::start() {
    return base_.start(weak_from_this());
  }

  void SendDisputeProtocolImpl::onIncomingStream(
      std::shared_ptr<Stream> stream) {
    BOOST_ASSERT(stream->remotePeerId().has_value());

    readRequest(stream);
  }

  void SendDisputeProtocolImpl::newOutgoingStream(
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
          network::streamReadBuffer(stream_res);
          auto self = wp.lock();
          if (not self) {
            cb(ProtocolError::GONE);
            return;
          }

          if (not stream_res.has_value()) {
            SL_VERBOSE(
                self->base_.logger(),
                "Error happened while connection over {} stream with {}: {}",
                self->protocolName(),
                peer_id,
                stream_res.error());
            cb(stream_res.as_failure());
            return;
          }
          const auto &stream_and_proto = stream_res.value();

          SL_DEBUG(self->base_.logger(),
                   "Established connection over {} stream with {}",
                   stream_and_proto.protocol,
                   peer_id);

          cb(std::move(stream_and_proto.stream));
        });
  }

  void SendDisputeProtocolImpl::readRequest(std::shared_ptr<Stream> stream) {
    auto read_writer = std::make_shared<ScaleMessageReadWriter>(stream);

    SL_DEBUG(base_.logger(),
             "Read request from incoming {} stream with {}",
             protocolName(),
             stream->remotePeerId().value());

    read_writer->read<DisputeMessage>([stream, wp = weak_from_this()](
                                          auto &&dispute_request_res) mutable {
      auto self = wp.lock();
      if (not self) {
        stream->reset();
        return;
      }

      if (not dispute_request_res.has_value()) {
        SL_VERBOSE(self->base_.logger(),
                   "Error at read request from incoming {} stream with {}: {}",
                   self->protocolName(),
                   stream->remotePeerId().value(),
                   dispute_request_res.error());

        stream->reset();
        return;
      }
      auto &dispute_request = dispute_request_res.value();

      if (self->base_.logger()->level() >= log::Level::VERBOSE) {
        std::string logmsg = fmt::format(
            "Dispute request is received from incoming {} stream with {}",
            self->protocolName(),
            stream->remotePeerId().value());
        // TODO: Make detailed log message here
        self->base_.logger()->verbose(std::move(logmsg));
      }

      //      auto dispute_response_res =
      //          self->dispute_observer_->onDisputeRequest(dispute_request);

      if (not dispute_response_res) {
        SL_VERBOSE(
            self->base_.logger(),
            "Error at execute request from incoming {} stream with {}: {}",
            self->protocolName(),
            stream->remotePeerId().value(),
            dispute_response_res.error());

        stream->reset();
        return;
      }

      // TODO:
      //  temporary store requests and change down reputation of malicious peers

      self->writeResponse(std::move(stream));
    });
  }

  void SendDisputeProtocolImpl::writeResponse(std::shared_ptr<Stream> stream) {
    auto read_writer = std::make_shared<ScaleMessageReadWriter>(stream);

    static const DisputeResponse dispute_response{kagome::Empty()};

    read_writer->write(
        dispute_response,
        [stream = std::move(stream),
         wp = weak_from_this()](auto &&write_res) mutable {
          auto self = wp.lock();
          if (not self) {
            stream->reset();
            return;
          }

          if (not write_res.has_value()) {
            SL_VERBOSE(
                self->base_.logger(),
                "Error at writing response to incoming {} stream with {}: {}",
                self->protocolName(),
                stream->remotePeerId().value(),
                write_res.error());
            stream->reset();
            return;
          }

          stream->close([](auto &&...) {});
        });
  }

  void SendDisputeProtocolImpl::writeRequest(
      std::shared_ptr<Stream> stream,
      DisputeMessage dispute_request,
      std::function<void(outcome::result<void>)> &&cb) {
    auto read_writer = std::make_shared<ScaleMessageReadWriter>(stream);

    SL_DEBUG(base_.logger(),
             "Write request info outgoing {} stream with {}",
             protocolName(),
             stream->remotePeerId().value());

    read_writer->write(
        dispute_request,
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
                self->base_.logger(),
                "Error at write request into outgoing {} stream with {}: {}",
                self->protocolName(),
                stream->remotePeerId().value(),
                write_res.error());

            stream->reset();
            cb(write_res.as_failure());
            return;
          }

          SL_DEBUG(self->base_.logger(),
                   "Request written successful into outgoing {} stream with {}",
                   self->protocolName(),
                   stream->remotePeerId().value());

          cb(outcome::success());
        });
  }

  void SendDisputeProtocolImpl::readResponse(
      std::shared_ptr<Stream> stream,
      std::function<void(outcome::result<void>)> &&response_handler) {
    auto read_writer = std::make_shared<ScaleMessageReadWriter>(stream);

    SL_DEBUG(base_.logger(),
             "Read response from outgoing {} stream with {}",
             protocolName(),
             stream->remotePeerId().value());

    read_writer->read<DisputeResponse>([stream,
                                        wp = weak_from_this(),
                                        response_handler =
                                            std::move(response_handler)](
                                           auto &&block_response_res) mutable {
      auto self = wp.lock();
      if (not self) {
        stream->reset();
        response_handler(ProtocolError::GONE);
        return;
      }

      if (not block_response_res.has_value()) {
        SL_VERBOSE(self->base_.logger(),
                   "Error at read response from outgoing {} stream with {}: {}",
                   self->protocolName(),
                   stream->remotePeerId().value(),
                   block_response_res.error());

        stream->reset();
        response_handler(block_response_res.as_failure());
        return;
      }

      SL_DEBUG(self->base_.logger(),
               "Successful response read from outgoing {} stream with {}",
               self->protocolName(),
               stream->remotePeerId().value());

      stream->reset();
      response_handler(outcome::success());
    });
  }

  void SendDisputeProtocolImpl::request(
      const PeerId &peer_id,
      DisputeMessage dispute_request,
      std::function<void(outcome::result<void>)> &&response_handler) {
    //
  }

}  // namespace kagome::network
