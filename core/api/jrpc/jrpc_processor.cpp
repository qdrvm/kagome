/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/jrpc/jrpc_processor.hpp"

namespace kagome::api {

  JRPCProcessor::JRPCProcessor() {
    // register json format handler
    jsonrpc_handler_.RegisterFormatHandler(format_handler_);
  }

  void JRPCProcessor::registerHandler(const std::string &name, Method method) {
    auto &dispatcher = jsonrpc_handler_.GetDispatcher();
    dispatcher.AddMethod(name, std::move(method));
  }

  void JRPCProcessor::processData(const std::string &request,
                                  const ResponseHandler &cb) {
    auto &&formatted_response = jsonrpc_handler_.HandleRequest(request);
    std::string response(formatted_response->GetData(),
                         formatted_response->GetSize());
    cb(response);
  }
}  // namespace kagome::api
