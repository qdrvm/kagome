/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SCALE_RPC_READER_LIBP2P_HPP
#define KAGOME_SCALE_RPC_READER_LIBP2P_HPP

#include <functional>
#include <memory>

#include <outcome/outcome.hpp>
#include "libp2p/basic/message_read_writer.hpp"
#include "libp2p/basic/readwriter.hpp"
#include "scale/scale.hpp"

namespace kagome::network {
  /**
   * Works with RPC requests-responses, encoded into SCALE
   */
  struct ScaleRPCLibp2p {
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
          std::make_shared<libp2p::basic::MessageReadWriter>(read_writer);
      msg_read_writer->read([read_writer = std::move(read_writer),
                             msg_read_writer,
                             cb = std::move(cb),
                             error_cb = std::move(error_cb)](
                                auto &&vec_sptr_res) mutable {
        ScaleRPCLibp2p::read(
            std::move(read_writer),
            [msg_read_writer = std::move(msg_read_writer),
             cb = std::move(cb),
             error_cb](Request request) mutable -> void {
              auto response_res = cb(std::move(request));
              if (!response_res) {
                return;
              }

              auto encoded_response_res = scale::encode(response_res.value());
              if (!encoded_response_res) {
                return error_cb(encoded_response_res.error());
              }

              msg_read_writer->write(
                  encoded_response_res.value(),
                  [error_cb = std::move(error_cb)](auto &&write_res) {
                    if (!write_res) {
                      return error_cb(write_res.error());
                    }
                  });
            },
            error_cb);
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
                     std::function<void(Request)> cb,
                     std::function<void(outcome::result<void>)> error_cb) {
      auto msg_read_writer = std::make_shared<libp2p::basic::MessageReadWriter>(
          std::move(read_writer));
      msg_read_writer->read(
          [msg_read_writer, cb = std::move(cb), error_cb = std::move(error_cb)](
              auto &&vec_sptr_res) mutable {
            if (!vec_sptr_res) {
              return error_cb(vec_sptr_res.error());
            }

            auto request_res = scale::decode<Request>(*vec_sptr_res.value());
            if (!request_res) {
              return error_cb(request_res.error());
            }

            cb(std::move(request_res.value()));
          });
    }
  };
}  // namespace kagome::network

#endif  // KAGOME_SCALE_RPC_READER_LIBP2P_HPP
