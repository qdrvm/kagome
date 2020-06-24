/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_RPC_HPP
#define KAGOME_RPC_HPP

#include <functional>
#include <memory>

#include <outcome/outcome.hpp>
#include "common/buffer.hpp"
#include "libp2p/basic/readwriter.hpp"
#include "libp2p/host/host.hpp"
#include "libp2p/peer/peer_info.hpp"
#include "libp2p/peer/protocol.hpp"

namespace kagome::network {
  /**
   * Works with RPC requests-responses through the Libp2p
   * @tparam MessageReadWriterT - type of read-writer to be used; for example,
   * ScaleMessageReadWriter or ProtobufMessageReadWriter
   */
  template <typename MessageReadWriterT>
  struct RPC {
    /**
     * Read an RPC request and answer with a response
     * @tparam Request - type of the request to be read
     * @tparam Response - type of the response to be written
     * @param read_writer - channel, from which to read and to which to write
     * @param cb, which is called, when the request is read; it is expected that
     * this function will return a corresponding response
     * @param error_cb, which is called, when error happens during read/write or
     * message processing
     */
    template <typename Request, typename Response>
    static void read(std::shared_ptr<libp2p::basic::ReadWriter> read_writer,
                     std::function<outcome::result<Response>(Request)> cb,
                     std::function<void(outcome::result<void>)> error_cb) {
      auto msg_read_writer =
          std::make_shared<MessageReadWriterT>(std::move(read_writer));
      msg_read_writer->template read<Request>(
          [msg_read_writer, cb = std::move(cb), error_cb = std::move(error_cb)](
              auto &&request_res) mutable {
            if (!request_res) {
              return error_cb(request_res.error());
            }

            auto response_res = cb(std::move(request_res.value()));
            if (!response_res) {
              return error_cb(response_res.error());
            }

            msg_read_writer->template write<Response>(
                response_res.value(),
                [error_cb = std::move(error_cb)](auto &&write_res) {
                  if (!write_res) {
                    return error_cb(write_res.error());
                  }
                });
          });
    }

    /**
     * Read an RPC request
     * @tparam Request - type of the request to be read
     * @param read_writer - channel, from which to read
     * @param cb, which is called, when the request is read
     * @param error_cb, which is called, when error happens
     */
    template <typename Request>
    static void read(std::shared_ptr<libp2p::basic::ReadWriter> read_writer,
                     std::function<void(outcome::result<Request>)> cb) {
      auto msg_read_writer =
          std::make_shared<MessageReadWriterT>(std::move(read_writer));
      msg_read_writer->template read<Request>(
          [msg_read_writer, cb = std::move(cb)](auto &&msg_res) mutable {
            if (!msg_res) {
              return cb(msg_res.error());
            }

            cb(std::move(msg_res.value()));
          });
    }

    /**
     * Write an RPC request and wait for a response
     * @tparam Request - type of the request to be written
     * @tparam Response - type of the response to be read
     * @param host, using which to communicate with the network
     * @param peer_info of the peer we want to write the request to
     * @param protocol, over which we want to write the request to
     * @param request we want to write
     * @param cb, which is called, when a response arrives, or error happens
     */
    template <typename Request, typename Response>
    static void write(libp2p::Host &host,
                      const libp2p::peer::PeerInfo &peer_info,
                      const libp2p::peer::Protocol &protocol,
                      Request request,
                      std::function<void(outcome::result<Response>)> cb) {
      host.newStream(
          peer_info,
          protocol,
          [request = std::move(request),
           cb = std::move(cb)](auto &&stream_res) mutable {
            if (!stream_res) {
              return cb(stream_res.error());
            }

            auto stream = std::move(stream_res.value());
            spdlog::debug("Sending blocks request to {}", stream->remotePeerId().value().toBase58());
            auto read_writer = std::make_shared<MessageReadWriterT>(stream);
            read_writer->template write<Request>(
                request,
                [read_writer, stream = std::move(stream), cb = std::move(cb)](
                    auto &&write_res) mutable {
                  if (!write_res) {
                    stream->reset();
                    return cb(write_res.error());
                  }
                  spdlog::debug("Request to {} sent successfully", stream->remotePeerId().value().toBase58());

                  read_writer->template read<Response>(
                      [stream = std::move(stream),
                       cb = std::move(cb)](auto &&msg_res) {
                        if (!msg_res) {
                          stream->reset();
                          return cb(msg_res.error());
                        }

                        stream->close([](auto &&) {});
                        return cb(std::move(msg_res.value()));
                      });
                });
          });
    }

    /**
     * Write an RPC request
     * @tparam Request - type of the request to be written
     * @param host, using which to communicate with the network
     * @param peer_info of the peer we want to write the request to
     * @param protocol, over which we want to write the request to
     * @param request we want to write
     * @param cb, which is called, when a response arrives, or error happens
     */
    template <typename Request>
    static void write(libp2p::Host &host,
                      const libp2p::peer::PeerInfo &peer_info,
                      const libp2p::peer::Protocol &protocol,
                      Request request,
                      std::function<void(outcome::result<void>)> cb) {
      host.newStream(peer_info,
                     protocol,
                     [request = std::move(request),
                      cb = std::move(cb)](auto &&stream_res) mutable {
                       if (!stream_res) {
                         return cb(stream_res.error());
                       }

                       auto stream = std::move(stream_res.value());
                       auto read_writer =
                           std::make_shared<MessageReadWriterT>(stream);
                       read_writer->template write<Request>(
                           request,
                           [stream = std::move(stream),
                            cb = std::move(cb)](auto &&write_res) {
                             if (!write_res) {
                               stream->reset();
                               return cb(write_res.error());
                             }

                             stream->close([](auto &&) {});
                             return cb(outcome::success());
                           });
                     });
    }
  };
}  // namespace kagome::network

#endif  // KAGOME_RPC_HPP
