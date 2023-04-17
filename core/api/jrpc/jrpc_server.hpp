/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_API_JRPC_SERVER_HPP
#define KAGOME_API_JRPC_SERVER_HPP

#include <functional>

#include <jsonrpc-lean/dispatcher.h>
#include <jsonrpc-lean/response.h>
#include <jsonrpc-lean/value.h>

#include "outcome/outcome.hpp"

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
    virtual void registerHandler(const std::string &name,
                                 Method method,
                                 bool unsafe = false) = 0;

    /**
     * @return name of handlers
     */
    virtual std::vector<std::string> getHandlerNames() = 0;

    /**
     * Response callback type
     */
    using ResponseHandler = std::function<void(std::string_view)>;
    using FormatterHandler =
        std::function<void(outcome::result<std::string_view>)>;

    /**
     * @brief creates valid jsonrpc response
     * @param method_name is a name of the method in jsonrpc call
     * @param from is a data source
     * @param cb callback
     */
    virtual void processJsonData(std::string method_name,
                                 const jsonrpc::Request::Parameters &from,
                                 const FormatterHandler &cb) = 0;

    /**
     * @brief handles decoded network message
     * @param request json request string
     * @param cb callback
     */
    virtual void processData(std::string_view request,
                             bool allow_unsafe,
                             const ResponseHandler &cb) = 0;
  };

}  // namespace kagome::api

#endif  // KAGOME_API_JRPC_SERVER_HPP
