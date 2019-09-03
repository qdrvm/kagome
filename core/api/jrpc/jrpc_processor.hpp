/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_API_JRPC_JRPC_PROCESSOR_HPP
#define KAGOME_CORE_API_JRPC_JRPC_PROCESSOR_HPP

#include <jsonrpc-lean/server.h>

namespace kagome::api {
  class JRPCProcessor {
   public:
    JRPCProcessor();

    using Method = jsonrpc::MethodWrapper::Method;
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
    void processData(const std::string &request, const ResponseHandler &cb);

    jsonrpc::JsonFormatHandler format_handler_{};  ///< format handler instance
    jsonrpc::Server jsonrpc_handler_{};            ///< json rpc server instance
  };
}  // namespace kagome::api

#endif  // KAGOME_CORE_API_JRPC_JRPC_PROCESSOR_HPP
