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

#include "api/state/state_jrpc_processor.hpp"

#include "api/jrpc/value_converter.hpp"
#include "api/state/impl/state_jrpc_param_parser.hpp"

namespace kagome::api {

  StateJrpcProcessor::StateJrpcProcessor(
      std::shared_ptr<JRpcServer> server, std::shared_ptr<StateApi> api)
      : api_{std::move(api)}, server_{std::move(server)} {
    BOOST_ASSERT(api_ != nullptr);
    BOOST_ASSERT(server_ != nullptr);
  }

  void StateJrpcProcessor::registerHandlers() {
    server_->registerHandler(
        "state_getStorage",
        [this](const jsonrpc::Request::Parameters &params) -> jsonrpc::Value {
          StateJrpcParamParser parser;
          auto &&[key, at] = parser.parseGetStorageParams(params);
          auto &&res = at ? api_->getStorage(key, at.value())
                          : this->api_->getStorage(key);
          if (!res) {
            throw jsonrpc::Fault(res.error().message());
          }
          return makeValue(res.value());
        });
  }

}  // namespace kagome::api
