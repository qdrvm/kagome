/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/service/system/requests/name.hpp"

#include <boost/assert.hpp>

namespace kagome::api::system::request {

  Name::Name(std::shared_ptr<SystemApi> api) : api_(std::move(api)) {
    BOOST_ASSERT(api_ != nullptr);
  }

  outcome::result<void> Name::init(const jsonrpc::Request::Parameters &params) {
    if (!params.empty()) {
      throw jsonrpc::InvalidParametersFault("Method should not have params");
    }
    return outcome::success();
  }

  outcome::result<std::string> Name::execute() {
    // In Polkadot it is just hardcoded string of client's implementation name:
    // https://github.com/paritytech/polkadot/blob/c68aee352b84321b6a5691d38e20550577d60a45/cli/src/command.rs#L34
    // So I assume we can safely use our client's name ehre
    return "Quadrivium Kagome";
  }

}  // namespace kagome::api::system::request
