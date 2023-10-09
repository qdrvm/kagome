/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/noncopyable.hpp>
#include <mutex>
#include "api/jrpc/value_converter.hpp"

#include "api/jrpc/jrpc_processor.hpp"
#include "api/jrpc/jrpc_server_impl.hpp"
#include "api/service/state/state_api.hpp"

namespace kagome::api {

  /**
   * @brief Lightweit implementation of API stub
   */
  class JrpcProcessorStub : public JRpcProcessor {
   public:
    JrpcProcessorStub(std::shared_ptr<JRpcServer> server,
                      std::shared_ptr<ApiStub> api)
        : api_{std::move(api)}, server_{std::move(server)} {
      BOOST_ASSERT(api_ != nullptr);
      BOOST_ASSERT(server_ != nullptr);
    }

    ~JrpcProcessorStub() override = default;

    void registerHandlers() override {
      server_->registerHandler(
          "echo",
          [this](const jsonrpc::Request::Parameters &params) -> jsonrpc::Value {
            auto &&nonce = parseParams(params);
            auto &&res = api_->echo(nonce);
            if (!res) {
              throw jsonrpc::Fault(res.error().message());
            }
            return jsonrpc::Value(res.value());
          });
    }

   private:
    std::shared_ptr<ApiStub> api_;
    std::shared_ptr<JRpcServer> server_;

    [[nodiscard]] int64_t parseParams(
        const jsonrpc::Request::Parameters &params) const {
      if (params.size() != 1) {
        throw jsonrpc::InvalidParametersFault("Incorrect number of params");
      }
      auto &param0 = params[0];
      if (!param0.IsInteger64() && !param0.IsInteger32()) {
        throw jsonrpc::InvalidParametersFault(
            "Single parameter must be integer");
      }
      return param0.AsInteger64();
    }
  };

}  // namespace kagome::api
