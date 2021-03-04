/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
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

  outcome::result<std::vector<std::string>> Methods::execute() {
    return api_->methods();
  }

}  // namespace kagome::api::rpc::request
