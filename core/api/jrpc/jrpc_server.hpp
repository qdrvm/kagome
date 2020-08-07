/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_API_JRPC_SERVER_HPP
#define KAGOME_API_JRPC_SERVER_HPP

#include <jsonrpc-lean/dispatcher.h>
#include <jsonrpc-lean/response.h>
#include <jsonrpc-lean/value.h>

#include <functional>

namespace kagome::api {

  /**
   * Instance of json rpc server, allows to register callbacks for rpc methods
   * and then invoke them
   */
  class JRpcServer {
   public:
    using Method = jsonrpc::MethodWrapper::Method;

    virtual ~JRpcServer() = default;

    /**
     * @brief registers rpc request handler lambda
     * @param name rpc method name
     * @param method handler functor
     */
    virtual void registerHandler(const std::string &name, Method method) = 0;

    /**
     * Response callback type
     */
    using ResponseHandler = std::function<void(const std::string &)>;

    /**
     * @brief creates valid jsonrpc response
     * @param from is a data source
     * @param cb callback
     */
    virtual void processJsonData(const jsonrpc::Value &from,
                                 const ResponseHandler &cb) = 0;

    /**
     * @brief handles decoded network message
     * @param request json request string
     * @param cb callback
     */
    virtual void processData(std::string_view request,
                             const ResponseHandler &cb) = 0;
  };

}  // namespace kagome::api

#endif  // KAGOME_API_JRPC_SERVER_HPP
