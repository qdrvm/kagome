/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/service/rpc/requests/methods.hpp"

#include "api/service/rpc/rpc_api.hpp"

namespace kagome::api::rpc::request {

  outcome::result<void> Methods::init(
      const jsonrpc::Request::Parameters &params) {
    if (!params.empty()) {
      throw jsonrpc::InvalidParametersFault("Method should not have params");
    }

    return outcome::success();
  }

  outcome::result<primitives::RpcMethods> Methods::execute() {
    OUTCOME_TRY(methods, api_->methods());
    primitives::RpcMethods result;
    result.methods = std::move(methods);
    return result;
  }

}  // namespace kagome::api::rpc::request
