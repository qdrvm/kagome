/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_API_JRPC_SERVER_IMPL_HPP
#define KAGOME_API_JRPC_SERVER_IMPL_HPP

#include <jsonrpc-lean/server.h>

#include "api/jrpc/jrpc_server.hpp"
#include "metrics/metrics.hpp"

namespace kagome::api {

  class JRpcServerImpl : public JRpcServer {
   public:
    enum class Error {
      JSON_FORMAT_FAILED = 1,
    };

    JRpcServerImpl();

    void registerHandler(const std::string &name,
                         Method method,
                         bool unsafe) override;

    /**
     * @return name of handlers
     */
    std::vector<std::string> getHandlerNames() override;

    void processData(std::string_view request,
                     bool allow_unsafe,
                     const ResponseHandler &cb) override;

    /**
     * @brief creates a valid jsonrpc response and passes it to \arg cb
     * @param from is a data source
     * @param cb callback
     */
    void processJsonData(std::string method_name,
                         const jsonrpc::Request::Parameters &from,
                         const FormatterHandler &cb) override;

   private:
    /// json rpc server instance
    jsonrpc::Server jsonrpc_handler_{};
    /// json rpc server instance for subset of safe methods
    jsonrpc::Server jsonrpc_handler_safe_{};
    /// format handler instance
    jsonrpc::JsonFormatHandler format_handler_{};

    // Metrics
    metrics::RegistryPtr metrics_registry_ = metrics::createRegistry();
    metrics::Counter *metric_rpc_requests_count_;
  };

}  // namespace kagome::api

OUTCOME_HPP_DECLARE_ERROR(kagome::api, JRpcServerImpl::Error);

#endif  // KAGOME_API_JRPC_SERVER_IMPL_HPP
