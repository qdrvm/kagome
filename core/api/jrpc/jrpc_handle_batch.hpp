/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_API_JRPC_HANDLE_BATCH_HPP
#define KAGOME_API_JRPC_HANDLE_BATCH_HPP

#include <memory>
#include <string>

namespace jsonrpc {
  class FormattedData;
  class Server;
}  // namespace jsonrpc

namespace kagome::api {
  /**
   * Handles single or batch requests.
   */
  class JrpcHandleBatch {
   public:
    /**
     * Construct response for single or batch request.
     */
    JrpcHandleBatch(jsonrpc::Server &handler, std::string_view request);

    /**
     * Get response.
     */
    std::string_view response() const;

   private:
    /**
     * Single response buffer returned by `jsonrpc::Server::HandleRequest`.
     */
    std::shared_ptr<jsonrpc::FormattedData> formatted_;
    /**
     * Combined batch responses buffer.
     */
    std::string batch_;
  };
}  // namespace kagome::api

#endif  // KAGOME_API_JRPC_HANDLE_BATCH_HPP
