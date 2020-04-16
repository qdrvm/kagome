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
