/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/service/child_state/requests/get_keys_paged.hpp"

#include "scale/scale.hpp"

namespace kagome::api::child_state::request {

  outcome::result<void> GetKeysPaged::init(
      const jsonrpc::Request::Parameters &params) {
    // getKeysPaged(childKey, prefix, count, [opt]startKey, [opt]at)

    if (params.size() > 5 or params.size() < 3) {
      throw jsonrpc::InvalidParametersFault("Incorrect number of params");
    }
    // childKey
    auto &param0 = params[0];

    if (not param0.IsString() or param0.IsNil()) {
      throw jsonrpc::InvalidParametersFault(
          "Parameter '[child_storage_key]' must be a hex string");
    }

    OUTCOME_TRY(child_storage_key, common::unhexWith0x(param0.AsString()));
    child_storage_key_ = common::Buffer(std::move(child_storage_key));

    auto &param1 = params[1];

    if (not param1.IsString() and not param1.IsNil()) {
      throw jsonrpc::InvalidParametersFault(
          "Parameter '[prefix]' must be a hex string");
    }

    if (param1.IsNil()) {
      prefix_ = std::nullopt;
    } else if (param1.IsString()) {
      OUTCOME_TRY(key, common::unhexWith0x(param1.AsString()));
      prefix_ = common::Buffer(std::move(key));
    } else {
      throw jsonrpc::InvalidParametersFault(
          "Parameter '[prefix]' must be a hex string");
    }

    if (not params[2].IsInteger32()) {
      throw jsonrpc::InvalidParametersFault(
          "Parameter '[key_amount]' must be a uint32_t");
    }

    keys_amount_ = params[2].AsInteger32();
    if (params.size() == 3) {
      return outcome::success();
    }

    // process prev_key param
    if (not params[3].IsString()) {
      throw jsonrpc::InvalidParametersFault(
          "Parameter '[prev_key]' must be a hex string representation of an "
          "encoded optional byte sequence");
    }
    OUTCOME_TRY(prev_key, common::unhexWith0x(params[3].AsString()));
    prev_key_ = common::Buffer{prev_key};

    if (params.size() == 4) {
      return outcome::success();
    }

    // process at param
    if (not params[4].IsString()) {
      throw jsonrpc::InvalidParametersFault(
          "Parameter '[at]' must be a hex string representation of an encoded "
          "optional byte sequence");
    }
    OUTCOME_TRY(at_span, common::unhexWith0x(params[4].AsString()));
    OUTCOME_TRY(at, primitives::BlockHash::fromSpan(at_span));
    at_ = at;

    return outcome::success();
  }

  outcome::result<std::vector<common::Buffer>> GetKeysPaged::execute() {
    return api_->getKeysPaged(
        child_storage_key_, prefix_, keys_amount_, prev_key_, at_);
  }
}  // namespace kagome::api::child_state::request