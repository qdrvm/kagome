/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/service/system/requests/chain.hpp"

#include <boost/assert.hpp>

#include "api/service/system/system_api.hpp"

namespace kagome::api::system::request {

  Chain::Chain(std::shared_ptr<SystemApi> api) : api_(std::move(api)) {
    BOOST_ASSERT(api_ != nullptr);
  }

  outcome::result<void> Chain::init(
      const jsonrpc::Request::Parameters &params) {
    if (!params.empty()) {
      throw jsonrpc::InvalidParametersFault("Method should not have params");
    }
    return outcome::success();
  }

  outcome::result<std::string> Chain::execute() {
    return api_->getConfig()->name();
  }

}  // namespace kagome::api::system::request
