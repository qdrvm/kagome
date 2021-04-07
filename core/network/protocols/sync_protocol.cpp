/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/protocols/sync_protocol.hpp"

#include <boost/assert.hpp>

#include "common/visitor.hpp"
#include "network/adapters/protobuf_block_request.hpp"
#include "network/adapters/protobuf_block_response.hpp"
#include "network/common.hpp"
#include "network/helpers/protobuf_message_read_writer.hpp"
#include "network/rpc.hpp"
#include "network/types/blocks_request.hpp"
#include "network/types/blocks_response.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::network, SyncProtocol::Error, e) {
  using E = kagome::network::SyncProtocol::Error;
  switch (e) {
    case E::CAN_NOT_CREATE_STATUS:
      return "Can not create status";
    case E::GONE:
      return "Protocol was switched off";
  }
  return "Unknown error";
}

namespace kagome::network {

  SyncProtocol::SyncProtocol(
      libp2p::Host &host,
      const application::ChainSpec &chain_spec,
      std::shared_ptr<SyncProtocolObserver> sync_observer)
      : host_(host), sync_observer_(std::move(sync_observer)) {
    BOOST_ASSERT(sync_observer_ != nullptr);
    const_cast<Protocol &>(protocol_) =
        fmt::format(kSyncProtocol.data(), chain_spec.protocolId());
  }

  bool SyncProtocol::start() {
    host_.setProtocolHandler(protocol_, [wp = weak_from_this()](auto &&stream) {
      if (auto self = wp.lock()) {
        if (auto peer_id = stream->remotePeerId()) {
          self->log_->trace("Handled {} protocol stream from: {}",
                            self->protocol_,
                            peer_id.value().toHex());
          self->onIncomingStream(std::forward<decltype(stream)>(stream));
          return;
        }
        self->log_->warn("Handled {} protocol stream from unknown peer",
                         self->protocol_);
      }

      RPC<ProtobufMessageReadWriter>::read<BlocksRequest, BlocksResponse>(
          stream,
          [wp = std::move(wp),
           stream](auto &&request) -> outcome::result<BlocksResponse> {
            auto self = wp.lock();
            if (not self) {
              return Error::GONE;
            }

            // std::bind didn't work :(
            std::string from = visit_in_place(
                request.from,
                [](primitives::BlockNumber number) {
                  return std::to_string(number);
                },
                [](const primitives::BlockHash &hash) { return hash.toHex(); });
            self->log_->debug(
                "Received request from peer {} requesting blocks from {} to "
                "{}",
                stream->remotePeerId().value().toBase58(),
                from,
                request.to->toHex());
            return self->sync_observer_->onBlocksRequest(
                std::forward<decltype(request)>(request));
          },
          [wp = std::move(wp), stream](auto &&err) -> outcome::result<void> {
            auto self = wp.lock();
            if (not self) {
              return Error::GONE;
            }
            self->log_->error(
                "error happened while processing message over Sync protocol: "
                "{}",
                err.error().message());
            stream->reset();
            return err.as_failure();
          });
    });
    return true;
  }  // namespace kagome::network

  bool SyncProtocol::stop() {
    return true;
  }

  void SyncProtocol::onIncomingStream(std::shared_ptr<Stream> stream) {
    BOOST_ASSERT(stream->remotePeerId().has_value());

    BOOST_ASSERT(false);
  }

  void SyncProtocol::newOutgoingStream(
      const PeerInfo &peer_info,
      std::function<void(outcome::result<std::shared_ptr<Stream>>)> &&cb) {
    host_.newStream(peer_info,
                    protocol_,
                    [wp = weak_from_this(),
                     peer_id = peer_info.id,
                     cb = std::move(cb)](auto &&stream_res) mutable {
                      auto self = wp.lock();
                      if (not self) {
                        cb(Error::GONE);
                        return;
                      }

                      if (not stream_res.has_value()) {
                        cb(stream_res.as_failure());
                        return;
                      }

                      auto &stream = stream_res.value();
                      cb(std::move(stream));
                    });
  }

  void SyncProtocol::readRequest(
      std::shared_ptr<Stream> stream,
      std::function<void(outcome::result<std::shared_ptr<Stream>>)> &&cb) {
    auto read_writer = std::make_shared<ProtobufMessageReadWriter>(stream);

    read_writer->read<BlocksRequest>(
        [stream, wp = weak_from_this(), cb = std::move(cb)](
            auto &&remote_status_res) mutable {
          auto self = wp.lock();
          if (not self) {
            stream->reset();
            cb(Error::GONE);
            return;
          }

          if (not remote_status_res.has_value()) {
            self->log_->error("Error while reading request: {}",
                              remote_status_res.error().message());
            stream->reset();
            cb(remote_status_res.as_failure());
            return;
          }

          BlocksResponse block_response;  // TODO make BlocksResponse

          self->writeResponse(std::move(stream), block_response, std::move(cb));
        });
  }

  void SyncProtocol::writeRequest(
      std::shared_ptr<Stream> stream,
      BlocksRequest block_request,
      std::function<void(outcome::result<std::shared_ptr<Stream>>)> &&cb) {
    auto read_writer = std::make_shared<ProtobufMessageReadWriter>(stream);

    read_writer->write(block_request,
                       [stream, wp = weak_from_this(), cb = std::move(cb)](
                           auto &&write_res) mutable {
                         auto self = wp.lock();
                         if (not self) {
                           stream->reset();
                           cb(Error::GONE);
                           return;
                         }

                         if (not write_res.has_value()) {
                           self->log_->error(
                               "Error while writing block request: {}",
                               write_res.error().message());
                           stream->reset();
                           cb(write_res.as_failure());
                           return;
                         }

                         self->readResponse(std::move(stream));
                       });
  }

  void SyncProtocol::readResponse(std::shared_ptr<Stream> stream) {
    auto read_writer = std::make_shared<ProtobufMessageReadWriter>(stream);

    read_writer->read<BlocksResponse>(
        [stream, wp = weak_from_this()](auto &&block_response_res) mutable {
          auto self = wp.lock();
          if (not self) {
            stream->reset();
            return;
          }

          if (not block_response_res) {
            self->log_->error("Error while reading block response: {}",
                              block_response_res.error().message());
            stream->reset();
            return;
          }

          auto peer_id = stream->remotePeerId().value();
          auto &blocks_response = block_response_res.value();

          // TODO consume BlocksResponse
          std::ignore = blocks_response;

          stream->close([](auto &&...) {});
        });
  }

  void SyncProtocol::writeResponse(
      std::shared_ptr<Stream> stream,
      const BlocksResponse &block_response,
      std::function<void(outcome::result<std::shared_ptr<Stream>>)> &&cb) {
    auto read_writer = std::make_shared<ProtobufMessageReadWriter>(stream);

    read_writer->write(
        block_response,
        [stream = std::move(stream), wp = weak_from_this(), cb = std::move(cb)](
            auto &&write_res) mutable {
          auto self = wp.lock();
          if (not self) {
            stream->reset();
            if (cb) cb(Error::GONE);
            return;
          }

          if (not write_res.has_value()) {
            self->log_->error("Error while writing block response: {}",
                              write_res.error().message());
            stream->reset();
            if (cb) cb(write_res.as_failure());
            return;
          }

          if (cb) cb(std::move(stream));
        });
  }
}  // namespace kagome::network
