/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/jrpc/jrpc_server_impl.hpp"

#include "api/jrpc/custom_json_writer.hpp"
#include "api/jrpc/jrpc_handle_batch.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::api, JRpcServerImpl::Error, e) {
  using E = kagome::api::JRpcServerImpl::Error;
  switch (e) {
    case E::JSON_FORMAT_FAILED:
      return "Json format failed";
  }
  return "Unknown error";
}

namespace {
  constexpr auto rpcRequestsCountMetricName = "kagome_rpc_requests_count";
}

namespace kagome::api {

  JRpcServerImpl::JRpcServerImpl() {
    // register json format handler
    jsonrpc_handler_.RegisterFormatHandler(format_handler_);
    jsonrpc_handler_safe_.RegisterFormatHandler(format_handler_);

    // Register metrics
    metrics_registry_->registerCounterFamily(rpcRequestsCountMetricName,
                                             "Block height info of the chain");

    metric_rpc_requests_count_ =
        metrics_registry_->registerCounterMetric(rpcRequestsCountMetricName);
  }

  void JRpcServerImpl::registerHandler(const std::string &name,
                                       Method method,
                                       bool unsafe) {
    auto &dispatcher = jsonrpc_handler_.GetDispatcher();
    auto &dispatcher_safe = jsonrpc_handler_safe_.GetDispatcher();
    if (not unsafe) {
      dispatcher_safe.AddMethod(name, method);
    }
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
    using Fault = jsonrpc::Fault;

    JsonWriter writer;
    try {
      /**
       * Notification must omit "id" field.
       * But jsonrpc-lean writes "id" field if id is null/int/string.
       * So we pass bool.
       * https://github.com/xDimon/jsonrpc-lean/blob/6c093da8670d7bf56555f166f8b8151f33a5d741/include/jsonrpc-lean/jsonwriter.h#L169
       */
      constexpr bool id = false;
      Response response(std::move(method_name), from, id);
      response.Write(writer);

      auto &&formatted_response = writer.GetData();
      cb(std::string_view(formatted_response->GetData(),
                          formatted_response->GetSize()));

    } catch (const Fault &ex) {
      cb(outcome::failure(Error::JSON_FORMAT_FAILED));
    }

    metric_rpc_requests_count_->inc();
  }

  void JRpcServerImpl::processData(std::string_view request,
                                   bool allow_unsafe,
                                   const ResponseHandler &cb) {
    JrpcHandleBatch response(
        allow_unsafe ? jsonrpc_handler_ : jsonrpc_handler_safe_, request);
    cb(response.response());
  }

}  // namespace kagome::api
