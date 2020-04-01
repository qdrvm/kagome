/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_API_JRPC_SERVER_IMPL_HPP
#define KAGOME_API_JRPC_SERVER_IMPL_HPP

#include <jsonrpc-lean/server.h>

#include "api/jrpc/jrpc_server.hpp"

namespace kagome::api {

  class JRpcServerImpl: public JRpcServer {
   public:
    JRpcServerImpl();

    ~JRpcServerImpl() override = default;

    /**
     * @brief registers rpc request handler lambda
     * @param name rpc method name
     * @param method handler functor
     */
    void registerHandler(const std::string &name, Method method) override;

    /**
     * @brief handles decoded network message
     * @param request json request string
     * @param cb callback
     */
    void processData(std::string_view request, const ResponseHandler &cb) override;

   private:
    /// json rpc server instance
    jsonrpc::Server jsonrpc_handler_ {};
    /// format handler instance
    jsonrpc::JsonFormatHandler format_handler_{};
  };

}  // namespace kagome::api

#endif  // KAGOME_API_JRPC_SERVER_IMPL_HPP
