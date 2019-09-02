/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/scale_rpc_receiver.hpp"

#include "scale/scale.hpp"

namespace kagome::network {
  template <typename Request, typename Response>
  void ScaleRPCReceiver::receive(
      const std::shared_ptr<libp2p::basic::MessageReadWriter> &read_writer,
      std::function<outcome::result<Response>(Request)> cb,
      std::function<void(outcome::result<void>)> error_cb) {
    read_writer->read([read_writer,
                       cb = std::move(cb),
                       error_cb =
                           std::move(error_cb)](auto &&vec_sptr_res) mutable {
      receive(
          read_writer,
          [read_writer, cb = std::move(cb), error_cb](Request request) mutable {
            auto response_res = cb(std::move(request));
            if (!response_res) {
              return;
            }

            auto encoded_response_res = scale::encode(response_res.value());
            if (!encoded_response_res) {
              return error_cb(encoded_response_res.error());
            }

            read_writer->write(
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

  template <typename Request>
  void ScaleRPCReceiver::receive(
      const std::shared_ptr<libp2p::basic::MessageReadWriter> &read_writer,
      std::function<void(Request)> cb,
      std::function<void(outcome::result<void>)> error_cb) {
    read_writer->read(
        [read_writer, cb = std::move(cb), error_cb = std::move(error_cb)](
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
}  // namespace kagome::network
