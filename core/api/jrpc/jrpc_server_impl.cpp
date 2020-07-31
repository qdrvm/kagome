/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
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
