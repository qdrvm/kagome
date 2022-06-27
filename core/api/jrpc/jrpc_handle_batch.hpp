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
  class JrpcHandleBatch {
   public:
    JrpcHandleBatch(jsonrpc::Server &handler, std::string_view request);

    std::string_view response() const;

   private:
    std::shared_ptr<jsonrpc::FormattedData> formatted_;
    std::string batch_;
  };
}  // namespace kagome::api

#endif  // KAGOME_API_JRPC_HANDLE_BATCH_HPP
