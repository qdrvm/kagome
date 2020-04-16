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

#ifndef KAGOME_JRPC_PROCESSOR_STUB_HPP
#define KAGOME_JRPC_PROCESSOR_STUB_HPP

#include "api/jrpc/value_converter.hpp"
#include <boost/noncopyable.hpp>
#include <mutex>

#include "api/jrpc/jrpc_processor.hpp"
#include "api/jrpc/jrpc_server_impl.hpp"
#include "api/state/state_api.hpp"

namespace kagome::api {

/**
 * @brief Lightweit implementation of API stub
 */
  class JrpcProcessorStub : public JRpcProcessor, private boost::noncopyable {
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

#endif  // KAGOME_JRPC_PROCESSOR_STUB_HPP
