/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/jrpc/jrpc_server_impl.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::api, JRpcServerImpl::Error, e) {
  using E = kagome::api::JRpcServerImpl::Error;
  switch (e) {
    case E::JSON_FORMAT_FAILED:
      return "Json format failed";
  }
  return "Unknown error";
}
namespace kagome::api {

  JRpcServerImpl::JRpcServerImpl() {
    // register json format handler
    jsonrpc_handler_.RegisterFormatHandler(format_handler_);
  }

  void JRpcServerImpl::registerHandler(const std::string &name, Method method) {
    auto &dispatcher = jsonrpc_handler_.GetDispatcher();
    dispatcher.AddMethod(name, std::move(method));
  }

  std::vector<std::string> JRpcServerImpl::getHandlerNames() {
    auto &dispatcher = jsonrpc_handler_.GetDispatcher();
    return dispatcher.GetMethodNames();
  }

  void JRpcServerImpl::processJsonData(std::string method_name,
                                       const jsonrpc::Request::Parameters &from,
                                       const FormatterHandler &cb) {
    /*
     * We need this because of spec format
     * https://github.com/w3f/PSPs/blob/psp-rpc-api/psp-002.md#state_subscribestorage-pubsub
     */
    using Response = jsonrpc::Request;
    using Value = jsonrpc::Value;
    using Fault = jsonrpc::Fault;

    auto writer = format_handler_.CreateWriter();
    try {
      Response response(std::move(method_name), from, Value(0));
      response.Write(*writer);

      auto &&formatted_response = writer->GetData();
      cb(std::string_view(formatted_response->GetData(),
                          formatted_response->GetSize()));
    } catch (const Fault &ex) {
      cb(outcome::failure(Error::JSON_FORMAT_FAILED));
    }
  }

  void JRpcServerImpl::processData(std::string_view request,
                                   const ResponseHandler &cb) {
    auto &&formatted_response =
        jsonrpc_handler_.HandleRequest(std::string(request));
    cb(std::string(formatted_response->GetData(),
                   formatted_response->GetSize()));
  }

}  // namespace kagome::api
