/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/service/state/requests/get_runtime_version.hpp"

namespace kagome::api::state::request {

  GetRuntimeVersion::GetRuntimeVersion(std::shared_ptr<StateApi> api)
      : api_(std::move(api)) {
    BOOST_ASSERT(!!api_);
  }

  outcome::result<void> GetRuntimeVersion::init(
      const jsonrpc::Request::Parameters &params) {
    if (params.size() > 1) {
      throw jsonrpc::InvalidParametersFault("Incorrect number of params");
    }

    auto const no_args = params.empty();
    auto const have_nil_arg = (!no_args && params[0].IsNil());
    auto const have_str_arg = (!no_args && params[0].IsString());

    if (no_args || have_nil_arg) {
      at_ = boost::none;
    } else if (have_str_arg) {
      auto &&at_str = params[0].AsString();
      OUTCOME_TRY(at_span, common::unhexWith0x(at_str));
      OUTCOME_TRY(at, primitives::BlockHash::fromSpan(at_span));
      at_ = at;
    } else {
      throw jsonrpc::InvalidParametersFault(
          "Parameter 'at' must be a hex string");
    }

    return outcome::success();
  }

  outcome::result<primitives::Version> GetRuntimeVersion::execute() {
    BOOST_ASSERT(api_);
    return api_->getRuntimeVersion(at_);
  }

}  // namespace kagome::api::state::request
