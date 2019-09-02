/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SCALE_RPC_RECEIVER_HPP
#define KAGOME_SCALE_RPC_RECEIVER_HPP

#include <functional>
#include <memory>

#include <outcome/outcome.hpp>
#include "libp2p/basic/message_read_writer.hpp"

namespace kagome::network {
  class ScaleRPCReceiver {
   public:
    template <typename Request, typename Response>
    static void receive(
        const std::shared_ptr<libp2p::basic::MessageReadWriter> &read_writer,
        std::function<outcome::result<Response>(Request)> cb,
        std::function<void(outcome::result<void>)> error_cb);

    template <typename Request>
    static void receive(
        const std::shared_ptr<libp2p::basic::MessageReadWriter> &read_writer,
        std::function<void(Request)> cb,
        std::function<void(outcome::result<void>)> error_cb);
  };
}  // namespace kagome::network

#endif  //KAGOME_SCALE_RPC_RECEIVER_HPP
