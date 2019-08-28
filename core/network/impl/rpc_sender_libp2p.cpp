/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/rpc_sender_libp2p.hpp"

#include "libp2p/basic/message_read_writer.hpp"

namespace kagome::network {
  using libp2p::basic::MessageReadWriter;

  RPCSenderLibp2p::RPCSenderLibp2p(libp2p::Host &host, common::Logger logger)
      : host_{host}, log_{std::move(logger)} {}

  void RPCSenderLibp2p::sendWithResponse(RPCInfoLibp2p rpc_info,
                                         BufferSPtr request,
                                         Callback cb) const {
    host_.newStream(
        rpc_info.peer_info,
        rpc_info.protocol,
        [self{shared_from_this()},
         request{std::move(request)},
         cb{std::move(cb)}](auto &&stream_res) mutable {
          if (!stream_res) {
            self->log_->error("cannot open a stream");
            return cb(stream_res.error());
          }

          auto read_writer = std::make_shared<MessageReadWriter>(
              std::move(stream_res.value()));
          read_writer->write(
              *request,
              [self{std::move(self)}, request, read_writer, cb{std::move(cb)}](
                  auto write_res) mutable {
                self->receive(std::move(write_res), read_writer, std::move(cb));
              });
        });
  }

  void RPCSenderLibp2p::receive(
      outcome::result<size_t> send_res,
      const std::shared_ptr<MessageReadWriter> &read_writer,
      Callback cb) const {
    if (!send_res) {
      log_->error("cannot write a request to stream");
      return cb(send_res.error());
    }

    read_writer->read(
        [self{shared_from_this()}, cb{std::move(cb)}](auto read_res) {
          if (!read_res) {
            self->log_->error("cannot read a response: {}",
                              read_res.error().message());
            return cb(read_res.error());
          }

          return cb(std::move(read_res.value()));
        });
  }

  void RPCSenderLibp2p::sendWithoutResponse(
      RPCInfoLibp2p rpc_info,
      BufferSPtr request,
      std::function<void(outcome::result<void>)> cb) const {
    host_.newStream(
        rpc_info.peer_info,
        rpc_info.protocol,
        [self{shared_from_this()},
         request{std::move(request)},
         cb{std::move(cb)}](auto &&stream_res) mutable {
          if (!stream_res) {
            self->log_->error("cannot open a stream");
            return cb(stream_res.error());
          }

          auto read_writer = std::make_shared<MessageReadWriter>(
              std::move(stream_res.value()));
          read_writer->write(
              *request,
              [self{std::move(self)}, request, read_writer, cb{std::move(cb)}](
                  auto write_res) {
                if (!write_res) {
                  return cb(std::move(write_res.error()));
                }
                return cb(outcome::success());
              });
        });
  }
}  // namespace kagome::network
