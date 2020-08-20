/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/service/system/requests/version.hpp"

namespace kagome::api::system::request {

  Version::Version(std::shared_ptr<SystemApi> api) : api_(std::move(api)) {
    BOOST_ASSERT(api_ != nullptr);
  }

  outcome::result<void> Version::init(
      const jsonrpc::Request::Parameters &params) {
    return outcome::success();
  }

  outcome::result<std::string> Version::execute() {
    // TODO(xDimon): Ensure if implementation is correct, and remove exception
    throw jsonrpc::InternalErrorFault(
        "Internal error: method is known, but not yet implemented");

    return api_->getConfig()->id();
  }

}  // namespace kagome::api::system::request
