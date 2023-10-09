/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/service/system/requests/properties.hpp"

#include "api/service/system/system_api.hpp"

namespace kagome::api::system::request {

  outcome::result<void> Properties::init(
      const jsonrpc::Request::Parameters &params) {
    if (!params.empty()) {
      throw jsonrpc::InvalidParametersFault("Method should not have params");
    }
    return outcome::success();
  }

  outcome::result<std::map<std::string, std::string>> Properties::execute() {
    return api_->getConfig()->properties();
  }

}  // namespace kagome::api::system::request
