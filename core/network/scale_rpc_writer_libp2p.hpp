/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SCALE_RPC_WRITER_LIBP2P_HPP
#define KAGOME_SCALE_RPC_WRITER_LIBP2P_HPP

#include <functional>

#include <outcome/outcome.hpp>
#include "common/buffer.hpp"
#include "libp2p/basic/message_read_writer.hpp"
#include "libp2p/host/host.hpp"
#include "libp2p/peer/peer_info.hpp"
#include "libp2p/peer/protocol.hpp"
#include "scale/scale.hpp"

namespace kagome::network {
  /**
   * Works with RPC requests-responses, encoded into SCALE
   */
  struct ScaleRPCWriterLibp2p {
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
      auto encoded_request_res = scale::encode(request);
      if (!encoded_request_res) {
        return cb(encoded_request_res.error());
      }
      auto request_buf = std::make_shared<common::Buffer>(
          std::move(encoded_request_res.value()));

      host.newStream(
          peer_info,
          protocol,
          [request = std::move(request_buf),
           cb = std::move(cb)](auto &&stream_res) mutable {
            if (!stream_res) {
              return cb(stream_res.error());
            }

            auto stream = std::move(stream_res.value());
            auto read_writer =
                std::make_shared<libp2p::basic::MessageReadWriter>(stream);
            read_writer->write(
                *request,
                [request,
                 read_writer,
                 stream = std::move(stream),
                 cb = std::move(cb)](auto &&write_res) mutable {
                  if (!write_res) {
                    stream->reset();
                    return cb(write_res.error());
                  }

                  read_writer->read([stream = std::move(stream),
                                     cb = std::move(cb)](auto &&read_res) {
                    if (!read_res) {
                      stream->reset();
                      return cb(read_res.error());
                    }

                    auto response_res =
                        scale::decode<Response>(read_res.value());
                    if (!response_res) {
                      stream->reset();
                      return cb(response_res.error());
                    }

                    stream->close();
                    return cb(std::move(response_res.value()));
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
      auto encoded_request_res = scale::encode(request);
      if (!encoded_request_res) {
        return cb(encoded_request_res.error());
      }
      auto request_buf = std::make_shared<common::Buffer>(
          std::move(encoded_request_res.value()));

      host.newStream(
          peer_info,
          protocol,
          [request = std::move(request_buf),
           cb = std::move(cb)](auto &&stream_res) mutable {
            if (!stream_res) {
              return cb(stream_res.error());
            }

            auto stream = std::move(stream_res.value());
            auto read_writer =
                std::make_shared<libp2p::basic::MessageReadWriter>(stream);
            read_writer->write(*request,
                               [request,
                                stream = std::move(stream),
                                cb = std::move(cb)](auto &&write_res) {
                                 if (!write_res) {
                                   stream->reset();
                                   return cb(write_res.error());
                                 }

                                 stream->close();
                                 return cb(outcome::success());
                               });
          });
    }
  };
}  // namespace kagome::network

#endif  // KAGOME_SCALE_RPC_WRITER_LIBP2P_HPP
