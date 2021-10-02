/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/protocols/sync_protocol_impl.hpp"

#include "common/visitor.hpp"
#include "network/adapters/protobuf_block_request.hpp"
#include "network/adapters/protobuf_block_response.hpp"
#include "network/common.hpp"
#include "network/helpers/protobuf_message_read_writer.hpp"
#include "network/rpc.hpp"
#include "network/types/blocks_request.hpp"
#include "network/types/blocks_response.hpp"
#include "network/impl/protocols/protocol_error.hpp"

namespace kagome::network {

  SyncProtocolImpl::SyncProtocolImpl(
      libp2p::Host &host,
      const application::ChainSpec &chain_spec,
      std::shared_ptr<SyncProtocolObserver> sync_observer)
      : host_(host), sync_observer_(std::move(sync_observer)) {
    BOOST_ASSERT(sync_observer_ != nullptr);
    const_cast<Protocol &>(protocol_) =
        fmt::format(kSyncProtocol.data(), chain_spec.protocolId());
  }

  bool SyncProtocolImpl::start() {
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

  bool SyncProtocolImpl::stop() {
    return true;
  }

  void SyncProtocolImpl::onIncomingStream(std::shared_ptr<Stream> stream) {
    BOOST_ASSERT(stream->remotePeerId().has_value());

    readRequest(stream);
  }

  void SyncProtocolImpl::newOutgoingStream(
      const PeerInfo &peer_info,
      std::function<void(outcome::result<std::shared_ptr<Stream>>)> &&cb) {
    SL_DEBUG(log_,
             "Connect for {} stream with {}",
             protocol_,
             peer_info.id.toBase58());

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
                peer_id.toBase58(),
                stream_res.error().message());
            cb(stream_res.as_failure());
            return;
          }
          auto &stream = stream_res.value();

          SL_DEBUG(self->log_,
                   "Established connection over {} stream with {}",
                   self->protocol_,
                   peer_id.toBase58());

          cb(std::move(stream));
        });
  }

  void SyncProtocolImpl::readRequest(std::shared_ptr<Stream> stream) {
    auto read_writer = std::make_shared<ProtobufMessageReadWriter>(stream);

    SL_DEBUG(log_,
             "Read request from incoming {} stream with {}",
             protocol_,
             stream->remotePeerId().value().toBase58());

    read_writer->read<BlocksRequest>([stream, wp = weak_from_this()](
                                         auto &&block_request_res) mutable {
      auto self = wp.lock();
      if (not self) {
        stream->reset();
        return;
      }

      if (not block_request_res.has_value()) {
        SL_VERBOSE(self->log_,
                   "Error at read request from incoming {} stream with {}: {}",
                   self->protocol_,
                   stream->remotePeerId().value().toBase58(),
                   block_request_res.error().message());

        stream->reset();
        return;
      }
      auto &block_request = block_request_res.value();

      SL_VERBOSE(
          self->log_,
          "Block request is received from incoming {} stream with {}: "
          "id={} fields={} dir={} from={} to={} amount={}",
          self->protocol_,
          stream->remotePeerId().value().toBase58(),
          block_request.id,
          (uint8_t)block_request.fields,
          block_request.direction == Direction::ASCENDING ? "anc" : "desc",
          visit_in_place(
              block_request.from,
              [](const primitives::BlockHash &hash) { return hash.toHex(); },
              [](const primitives::BlockNumber &number) {
                return std::to_string(number);
              }),
          block_request.to.has_value() ? block_request.to.value().toHex()
                                       : "max",
          block_request.max.has_value()
              ? std::to_string(block_request.max.value())
              : "max");

      auto block_response_res =
          self->sync_observer_->onBlocksRequest(block_request);

      if (not block_response_res) {
        SL_VERBOSE(
            self->log_,
            "Error at execute request from incoming {} stream with {}: {}",
            self->protocol_,
            stream->remotePeerId().value().toBase58(),
            block_response_res.error().message());

        stream->reset();
        return;
      }
      auto &block_response = block_response_res.value();

      self->writeResponse(std::move(stream), block_response);
    });
  }

  void SyncProtocolImpl::writeResponse(std::shared_ptr<Stream> stream,
                                       const BlocksResponse &block_response) {
    auto read_writer = std::make_shared<ProtobufMessageReadWriter>(stream);

    read_writer->write(
        block_response,
        [stream = std::move(stream),
         wp = weak_from_this()](auto &&write_res) mutable {
          auto self = wp.lock();
          if (not self) {
            stream->reset();
            return;
          }

          if (not write_res.has_value()) {
            SL_VERBOSE(
                self->log_,
                "Error at writting response to incoming {} stream with {}: {}",
                self->protocol_,
                stream->remotePeerId().value().toBase58(),
                write_res.error().message());
            stream->reset();
            return;
          }

          stream->close([](auto &&...) {});
        });
  }

  void SyncProtocolImpl::writeRequest(
      std::shared_ptr<Stream> stream,
      BlocksRequest block_request,
      std::function<void(outcome::result<void>)> &&cb) {
    auto read_writer = std::make_shared<ProtobufMessageReadWriter>(stream);

    SL_DEBUG(log_,
             "Write request info outgoing {} stream with {}",
             protocol_,
             stream->remotePeerId().value().toBase58());

    read_writer->write(
        block_request,
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
                stream->remotePeerId().value().toBase58(),
                write_res.error().message());

            stream->reset();
            cb(write_res.as_failure());
            return;
          }

          SL_DEBUG(
              self->log_,
              "Request written successfuly into outgoing {} stream with {}",
              self->protocol_,
              stream->remotePeerId().value().toBase58());

          stream->close([](auto &&...) {});
          cb(outcome::success());
        });
  }

  void SyncProtocolImpl::readResponse(
      std::shared_ptr<Stream> stream,
      std::function<void(outcome::result<BlocksResponse>)> &&response_handler) {
    auto read_writer = std::make_shared<ProtobufMessageReadWriter>(stream);

    SL_DEBUG(log_,
             "Read response from outgoing {} stream with {}",
             protocol_,
             stream->remotePeerId().value().toBase58());

    read_writer->read<BlocksResponse>([stream,
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
        SL_VERBOSE(self->log_,
                   "Error at read response from outgoing {} stream with {}: {}",
                   self->protocol_,
                   stream->remotePeerId().value().toBase58(),
                   block_response_res.error().message());

        stream->reset();
        response_handler(block_response_res.as_failure());
        return;
      }
      auto &blocks_response = block_response_res.value();

      SL_DEBUG(self->log_,
               "Response read successfuly from outgoing {} stream with {}",
               self->protocol_,
               stream->remotePeerId().value().toBase58());

      stream->reset();
      response_handler(std::move(blocks_response));
    });
  }

  void SyncProtocolImpl::request(
      const PeerId &peer_id,
      BlocksRequest block_request,
      std::function<void(outcome::result<BlocksResponse>)> &&response_handler) {
    auto addresses_res =
        host_.getPeerRepository().getAddressRepository().getAddresses(peer_id);
    if (not addresses_res.has_value()) {
      response_handler(addresses_res.as_failure());
      return;
    }

    visit_in_place(
        block_request.from,
        [this](primitives::BlockNumber from) {
          SL_DEBUG(log_, "Requesting blocks: from {}", from);
        },
        [this, &block_request](const primitives::BlockHash &from) {
          if (not block_request.to) {
            SL_DEBUG(log_, "Requesting blocks: from {}", from.toHex());
          } else {
            SL_DEBUG(log_,
                     "Requesting blocks: from {}, to {}",
                     from.toHex(),
                     block_request.to->toHex());
          }
        });

    newOutgoingStream(
        {peer_id, addresses_res.value()},
        [wp = weak_from_this(),
         response_handler = std::move(response_handler),
         block_request = std::move(block_request)](auto &&stream_res) mutable {
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

          SL_DEBUG(self->log_,
                   "Established outgoing {} stream with {}",
                   self->protocol_,
                   stream->remotePeerId().value().toBase58());

          self->writeRequest(stream,
                             std::move(block_request),
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

                               if (not write_res.has_value()) {
                                 response_handler(write_res.as_failure());
                                 return;
                               }

                               self->readResponse(std::move(stream),
                                                  std::move(response_handler));
                             });
        });
  }

}  // namespace kagome::network
