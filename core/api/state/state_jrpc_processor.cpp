/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/state/state_jrpc_processor.hpp"

#include "api/jrpc/value_converter.hpp"
#include "api/state/impl/state_jrpc_param_parser.hpp"

namespace kagome::api {

  StateJrpcProcessor::StateJrpcProcessor(std::shared_ptr<StateApi> api)
      : api_{std::move(api)} {
    BOOST_ASSERT(api_ != nullptr);
  }

  void StateJrpcProcessor::registerHandlers() {
    registerHandler(
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
