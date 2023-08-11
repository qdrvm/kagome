/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/service/state/requests/call.hpp"

#include "scale/scale.hpp"

namespace kagome::api::state::request {

  Call::Call(std::shared_ptr<StateApi> api) : api_(std::move(api)) {
    BOOST_ASSERT(api_);
  };

  outcome::result<void> Call::init(const jsonrpc::Request::Parameters &params) {
    if (params.size() < 2 or params.size() > 3) {
      throw jsonrpc::InvalidParametersFault("Incorrect number of params");
    }
    auto &param0 = params[0];

    if (param0.IsString()) {
      method_ = param0.AsString();
    } else {
      throw jsonrpc::InvalidParametersFault(
          "Parameter '[method]' must be a string");
    }

    auto &param1 = params[1];
    if (param1.IsString()) {
      auto encoded_args = common::unhexWith0x(param1.AsString());
      if (encoded_args.has_value()) {
        data_ = common::Buffer(encoded_args.value());
      } else {
        throw jsonrpc::InvalidParametersFault(
            "Parameter '[data]' must be a hex-encoded string");
      }
    } else {
      throw jsonrpc::InvalidParametersFault(
          "Parameter '[data]' must be a hex-encoded string");
    }

    if (params.size() == 2) {
      return outcome::success();
    }

    auto &param2 = params[2];
    if (!param2.IsNil()) {
      // process at param
      if (not param2.IsString()) {
        throw jsonrpc::InvalidParametersFault(
            "Parameter '[at]' must be a hex string representation of an "
            "encoded "
            "optional byte sequence");
      }
      OUTCOME_TRY(at_span, common::unhexWith0x(param2.AsString()));
      OUTCOME_TRY(at, primitives::BlockHash::fromSpan(at_span));
      at_ = at;
    }

    return outcome::success();
  }

  outcome::result<common::Buffer> Call::execute() {
    return api_->call(method_, data_, at_);
  }
}  // namespace kagome::api::state::request
