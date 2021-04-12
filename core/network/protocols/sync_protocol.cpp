/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/protocols/sync_protocol.hpp"
//
//#include <boost/assert.hpp>
//
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
                            peer_id.value().toBase58());
          self->onIncomingStream(std::forward<decltype(stream)>(stream));
          return;
        }
        self->log_->warn("Handled {} protocol stream from unknown peer",
                         self->protocol_);
      }

      //      RPC<ProtobufMessageReadWriter>::read<BlocksRequest,
      //      BlocksResponse>(
      //          stream,
      //          [wp = std::move(wp),
      //           stream](auto &&request) -> outcome::result<BlocksResponse> {
      //            auto self = wp.lock();
      //            if (not self) {
      //              return Error::GONE;
      //            }
      //
      //            // std::bind didn't work :(
      //            std::string from = visit_in_place(
      //                request.from,
      //                [](primitives::BlockNumber number) {
      //                  return std::to_string(number);
      //                },
      //                [](const primitives::BlockHash &hash) { return
      //                hash.toHex(); });
      //            self->log_->debug(
      //                "Received request from peer {} requesting blocks from {}
      //                to "
      //                "{}",
      //                stream->remotePeerId().value().toBase58(),
      //                from,
      //                request.to->toHex());
      //            return self->sync_observer_->onBlocksRequest(
      //                std::forward<decltype(request)>(request));
      //          },
      //          [wp = std::move(wp), stream](auto &&err) ->
      //          outcome::result<void> {
      //            auto self = wp.lock();
      //            if (not self) {
      //              return Error::GONE;
      //            }
      //            self->log_->error(
      //                "error happened while processing message over Sync
      //                protocol: "
      //                "{}",
      //                err.error().message());
      //            stream->reset();
      //            return err.as_failure();
      //          });
    });
    return true;
  }

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
    log_->info(
        "Connect for {} stream with {}", protocol_, peer_info.id.toBase58());

    host_.newStream(
        peer_info,
        protocol_,
        [wp = weak_from_this(), peer_id = peer_info.id, cb = std::move(cb)](
            auto &&stream_res) mutable {
          auto self = wp.lock();
          if (not self) {
            SL_VERBOSE(
                self->log_,
                "Error happened while connection over {} stream with {}: {}",
                self->protocol_,
                peer_id.toBase58(),
                outcome::result<void>(Error::GONE).error().message());
            cb(Error::GONE);
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

          SL_VERBOSE(self->log_,
                     "Established connection over {} stream with {}",
                     self->protocol_,
                     peer_id.toBase58());

          auto &stream = stream_res.value();
          cb(std::move(stream));
        });
  }

  void SyncProtocol::readRequest(
      std::shared_ptr<Stream> stream,
      std::function<void(outcome::result<std::shared_ptr<Stream>>)> &&cb) {
    auto read_writer = std::make_shared<ProtobufMessageReadWriter>(stream);

    // read_writer->read<BlocksRequest>(
    //    [stream, wp = weak_from_this(), cb = std::move(cb)](
    //        auto &&remote_status_res) mutable {
    //      auto self = wp.lock();
    //      if (not self) {
    //        stream->reset();
    //        cb(Error::GONE);
    //        return;
    //      }
    //
    //      if (not remote_status_res.has_value()) {
    //        self->log_->error("Error while reading request: {}",
    //                          remote_status_res.error().message());
    //        stream->reset();
    //        cb(remote_status_res.as_failure());
    //        return;
    //      }
    //
    //      BlocksResponse block_response;  // TODO make BlocksResponse
    //
    //      self->writeResponse(std::move(stream), block_response,
    //      std::move(cb));
    //    });
  }

  void SyncProtocol::writeResponse(
      std::shared_ptr<Stream> stream,
      const BlocksResponse &block_response,
      std::function<void(outcome::result<std::shared_ptr<Stream>>)> &&cb) {
    auto read_writer = std::make_shared<ProtobufMessageReadWriter>(stream);

    // read_writer->write(
    //    block_response,
    //    [stream = std::move(stream), wp = weak_from_this(), cb =
    //    std::move(cb)](
    //        auto &&write_res) mutable {
    //      auto self = wp.lock();
    //      if (not self) {
    //        stream->reset();
    //        if (cb) cb(Error::GONE);
    //        return;
    //      }
    //
    //      if (not write_res.has_value()) {
    //        self->log_->error("Error while writing block response: {}",
    //                          write_res.error().message());
    //        stream->reset();
    //        if (cb) cb(write_res.as_failure());
    //        return;
    //      }
    //
    //      if (cb) cb(std::move(stream));
    //    });
  }

  void SyncProtocol::writeRequest(
      std::shared_ptr<Stream> stream,
      BlocksRequest block_request,
      std::function<void(outcome::result<void>)> &&cb) {
    auto read_writer = std::make_shared<ProtobufMessageReadWriter>(stream);

    log_->info("Write request info outgoing {} stream with {}",
               protocol_,
               stream->remotePeerId().value().toBase58());

    read_writer->write(
        block_request,
        [stream, wp = weak_from_this(), cb = std::move(cb)](
            auto &&write_res) mutable {
          auto self = wp.lock();
          if (not self) {
            stream->reset();
            cb(Error::GONE);
            return;
          }

          if (not write_res.has_value()) {
            self->log_->info(
                "Error at write request into outgoing {} stream with {}: {}",
                self->protocol_,
                stream->remotePeerId().value().toBase58(),
                write_res.error().message());

            stream->reset();
            cb(write_res.as_failure());
            return;
          }

          self->log_->info(
              "Request written successfuly into outgoing {} stream with {}",
              self->protocol_,
              stream->remotePeerId().value().toBase58());

          stream->close([](auto &&...) {});
          cb(outcome::success());
        });
  }

  void SyncProtocol::readResponse(
      std::shared_ptr<Stream> stream,
      std::function<void(outcome::result<BlocksResponse>)> &&response_handler) {
    auto read_writer = std::make_shared<ProtobufMessageReadWriter>(stream);

    log_->info("Read response from outgoing {} stream with {}",
               protocol_,
               stream->remotePeerId().value().toBase58());

    read_writer->read<BlocksResponse>(
        [stream,
         wp = weak_from_this(),
         response_handler =
             std::move(response_handler)](auto &&block_response_res) mutable {
          auto self = wp.lock();
          if (not self) {
            stream->reset();
            response_handler(Error::GONE);
            return;
          }

          if (not block_response_res.has_value()) {
            self->log_->info(
                "Error at read response from outgoing {} stream with {}: {}",
                self->protocol_,
                stream->remotePeerId().value().toBase58(),
                block_response_res.error().message());

            stream->reset();
            response_handler(block_response_res.as_failure());
            return;
          }
          auto &blocks_response = block_response_res.value();

          self->log_->info(
              "Response read successfuly from outgoing {} stream with {}",
              self->protocol_,
              stream->remotePeerId().value().toBase58());

          stream->reset();
          response_handler(std::move(blocks_response));
        });
  }

  void SyncProtocol::request(
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
          log_->debug("Requesting blocks: from {}", from);
        },
        [this, &block_request](const primitives::BlockHash &from) {
          if (not block_request.to) {
            log_->debug("Requesting blocks: from {}", from.toHex());
          } else {
            log_->debug("Requesting blocks: from {}, to {}",
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
            response_handler(Error::GONE);
            return;
          }

          self->log_->info("Established outgoing {} stream with {}",
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
                                 response_handler(Error::GONE);
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
