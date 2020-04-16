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

#include "api/jrpc/jrpc_server_impl.hpp"

namespace kagome::api {

  JRpcServerImpl::JRpcServerImpl() {
    // register json format handler
    jsonrpc_handler_.RegisterFormatHandler(format_handler_);
  }

  void JRpcServerImpl::registerHandler(const std::string &name, Method method) {
    auto &dispatcher = jsonrpc_handler_.GetDispatcher();
    dispatcher.AddMethod(name, std::move(method));
  }

  void JRpcServerImpl::processData(std::string_view request,
                               const ResponseHandler &cb) {
    auto &&formatted_response =
        jsonrpc_handler_.HandleRequest(std::string(request));
    std::string response(formatted_response->GetData(),
                         formatted_response->GetSize());
    cb(response);
  }

}  // namespace kagome::api
