/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/service/state/requests/subscribe_runtime_version.hpp"

namespace kagome::api::state::request {

  outcome::result<void> SubscribeRuntimeVersion::init(
      const jsonrpc::Request::Parameters &params) {
    if (!params.empty()) {
      throw jsonrpc::InvalidParametersFault("Method should not have params");
    }

    return outcome::success();
  }

  outcome::result<void> SubscribeRuntimeVersion::execute() {
    if (auto result = api_->subscribeRuntimeVersion(); not result.has_value()) {
      return result.as_failure();
    }
    return outcome::success();
  }

}  // namespace kagome::api::state::request
