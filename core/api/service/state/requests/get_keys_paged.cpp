/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/service/state/requests/get_keys_paged.hpp"

#include "scale/scale.hpp"

namespace kagome::api::state::request {

  outcome::result<void> GetKeysPaged::init(
      const jsonrpc::Request::Parameters &params) {
    if (params.size() > 4 or params.size() <= 1) {
      throw jsonrpc::InvalidParametersFault("Incorrect number of params");
    }
    auto &param0 = params[0];

    if (not param0.IsString() and not param0.IsNil()) {
      throw jsonrpc::InvalidParametersFault(
          "Parameter '[prefix]' must be a hex string");
    }

    if (param0.IsNil()) {
      prefix_ =
          std::nullopt;  // I suppose none here is better than empty Buffer
    } else if (param0.IsString()) {
      OUTCOME_TRY(key, common::unhexWith0x(param0.AsString()));
      prefix_.emplace(std::move(key));
    } else {
      throw jsonrpc::InvalidParametersFault(
          "Parameter '[prefix]' must be a hex string");
    }

    if (not params[1].IsInteger32()) {
      throw jsonrpc::InvalidParametersFault(
          "Parameter '[key_amount]' must be a uint32_t");
    }

    keys_amount_ = params[1].AsInteger32();
    if (params.size() == 2) {
      return outcome::success();
    }

    // process prev_key param
    if (not params[2].IsNil()) {
      if (not params[2].IsString()) {
        throw jsonrpc::InvalidParametersFault(
            "Parameter '[prev_key]' must be a hex string representation of an "
            "encoded optional byte sequence");
      }
      OUTCOME_TRY(prev_key, common::unhexWith0x(params[2].AsString()));
      prev_key_.emplace(std::move(prev_key));
    }

    if (params.size() == 3) {
      return outcome::success();
    }

    // process at param
    if (not params[3].IsString()) {
      throw jsonrpc::InvalidParametersFault(
          "Parameter '[at]' must be a hex string representation of an encoded "
          "optional byte sequence");
    }
    OUTCOME_TRY(at_span, common::unhexWith0x(params[3].AsString()));
    OUTCOME_TRY(at, primitives::BlockHash::fromSpan(at_span));
    at_ = at;

    return outcome::success();
  }

  outcome::result<std::vector<common::Buffer>> GetKeysPaged::execute() {
    return api_->getKeysPaged(prefix_, keys_amount_, prev_key_, at_);
  }

}  // namespace kagome::api::state::request
