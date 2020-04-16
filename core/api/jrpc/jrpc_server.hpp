/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef KAGOME_API_JRPC_SERVER_HPP
#define KAGOME_API_JRPC_SERVER_HPP

#include <jsonrpc-lean/dispatcher.h>

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
     * @brief handles decoded network message
     * @param request json request string
     * @param cb callback
     */
    virtual void processData(std::string_view request,
                             const ResponseHandler &cb) = 0;
  };

}  // namespace kagome::api

#endif  // KAGOME_API_JRPC_SERVER_HPP
