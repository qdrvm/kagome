/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/consensus_server_libp2p.hpp"

#include "libp2p/basic/message_read_writer.hpp"
#include "libp2p/connection/stream.hpp"
#include "network/impl/common.hpp"
#include "network/impl/scale_rpc_receiver.hpp"
#include "scale/scale.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::network, ConsensusServerLibp2p::Error, e) {
  using E = kagome::network::ConsensusServerLibp2p::Error;
  switch (e) {
    case E::UNEXPECTED_MESSAGE_TYPE:
      return "unexpected message type arrived over the sync protocol";
  }
  return "unknown error";
}

namespace kagome::network {
  using libp2p::basic::MessageReadWriter;

  ConsensusServerLibp2p::ConsensusServerLibp2p(libp2p::Host &host,
                                               common::Logger log)
      : host_{host}, log_{std::move(log)} {}

  void ConsensusServerLibp2p::start() const {
    host_.setProtocolHandler(kSyncProtocol,
                             [self{shared_from_this()}](auto stream) mutable {
                               self->handleSyncProto(std::move(stream));
                             });
  }

  void ConsensusServerLibp2p::setBlocksRequestHandler(
      BlocksRequestHandler handler) {
    blocks_request_handler_ = std::move(handler);
  }

  void ConsensusServerLibp2p::handleSyncProto(
      std::shared_ptr<libp2p::connection::Stream> stream) const {
    ScaleRPCReceiver::receive<NetworkMessage, NetworkMessage>(
        std::make_shared<MessageReadWriter>(stream),
        [self{shared_from_this()},
         stream](NetworkMessage msg) -> outcome::result<NetworkMessage> {
          switch (msg.type) {
            case NetworkMessage::Type::BLOCKS_REQUEST: {
              auto request_res = scale::decode<BlocksRequest>(msg.body);
              if (!request_res) {
                self->log_->error("cannot decode blocks request: {}",
                                  request_res.error().message());
                stream->reset();
                return request_res.error();
              }
              return self->handleBlocksRequest(request_res.value());
            }
            default:
              self->log_->error(
                  "unexpected message type arrived over the sync protocol");
              stream->reset();
              return Error::UNEXPECTED_MESSAGE_TYPE;
          }
        },
        [self{shared_from_this()}, stream](auto err) {
          self->log_->error(
              "error while receiving a message over sync protocol: {}",
              err.error().message());
          return stream->reset();
        });
  }

  outcome::result<NetworkMessage> ConsensusServerLibp2p::handleBlocksRequest(
      const BlocksRequest &request) const {
    auto response_res = blocks_request_handler_(request);
    if (!response_res) {
      log_->error("cannot process blocks request: {}",
                  response_res.error().message());
      return response_res.error();
    }

    auto encoded_response_res = scale::encode(std::move(response_res.value()));
    if (!encoded_response_res) {
      log_->error("cannot encode blocks response: {}",
                  encoded_response_res.error().message());
      return encoded_response_res.error();
    }

    return NetworkMessage{
        NetworkMessage::Type::BLOCKS_RESPONSE,
        common::Buffer{std::move(encoded_response_res.value())}};
  }
}  // namespace kagome::network
