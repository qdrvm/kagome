/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/service/system/requests/name.hpp"

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
    // It should be implementation name, according to
    // https://github.com/paritytech/substrate/blob/0227ff513a045305efee0d8c2cb6025bcefb69ff/client/rpc-api/src/system/helpers.rs#L29
    return "Soramitsu Kagome";
  }

}  // namespace kagome::api::system::request
