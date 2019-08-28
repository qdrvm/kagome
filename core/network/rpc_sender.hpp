/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_RPC_SENDER_HPP
#define KAGOME_RPC_SENDER_HPP

#include <functional>
#include <memory>

#include <outcome/outcome.hpp>
#include "common/buffer.hpp"

namespace kagome::network {
  /**
   * Helps to handle RPC model of communication
   * @tparam RPCInfo - implementation-specific structure, containing
   * information, needed to make an RPC call
   */
  template <typename RPCInfo>
  struct RPCSender {
    virtual ~RPCSender() = default;

    using BufferSPtr = std::shared_ptr<common::Buffer>;
    using Callback = std::function<void(
        outcome::result<std::shared_ptr<std::vector<uint8_t>>>)>;

    /**
     * Send a request and wait for response
     * @param rpc_info with information to make RPC call
     * @param request to be sent
     * @param cb to be called, when a response arrives, or an error happens
     */
    virtual void sendWithResponse(RPCInfo rpc_info,
                                  BufferSPtr request,
                                  Callback cb) const = 0;

    /**
     * Send a request and do not wait for response
     * @param rpc_info with information to make RPC call
     * @param request to be sent
     * @param cb to be called, when the request is sent, or an error happens
     */
    virtual void sendWithoutResponse(
        RPCInfo rpc_info,
        BufferSPtr request,
        std::function<void(outcome::result<void>)> cb) const = 0;
  };
}  // namespace kagome::network

#endif  // KAGOME_RPC_SENDER_HPP
