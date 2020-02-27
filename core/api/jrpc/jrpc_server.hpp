/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_API_JRPC_SERVER_HPP
#define KAGOME_API_JRPC_SERVER_HPP

#include <jsonrpc-lean/server.h>

namespace kagome::api {

  class JRpcServer {
   public:
    using Method = jsonrpc::MethodWrapper::Method;

    JRpcServer();

    virtual ~JRpcServer() = default;

    /**
     * @brief registers rpc request handler lambda
     * @param name rpc method name
     * @param method handler functor
     */
    void registerHandler(const std::string &name, Method method);

    /**
     * Response callback type
     */
    using ResponseHandler = std::function<void(const std::string &)>;

    /**
     * @brief handles decoded network message
     * @param request json request string
     * @param cb callback
     */
    void processData(std::string_view request, const ResponseHandler &cb);

   private:
    std::shared_ptr<jsonrpc::Server> jsonrpc_handler_ {};            ///< json rpc server instance
    jsonrpc::JsonFormatHandler format_handler_{};  ///< format handler instance
  };

}  // namespace kagome::api

#endif  // KAGOME_API_JRPC_SERVER_HPP
