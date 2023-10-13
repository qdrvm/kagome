/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/service/child_state/requests/get_keys.hpp"

#include "scale/scale.hpp"

namespace kagome::api::child_state::request {

  outcome::result<void> GetKeys::init(
      const jsonrpc::Request::Parameters &params) {
    // getKeys(childKey, prefix, [opt]at)

    if (params.size() > 3 or params.size() < 2) {
      throw jsonrpc::InvalidParametersFault("Incorrect number of params");
    }
    // childKey
    auto &param0 = params[0];

    if (not param0.IsString()) {
      throw jsonrpc::InvalidParametersFault(
          "Parameter '[child_storage_key]' must be a hex string");
    }

    OUTCOME_TRY(child_storage_key, common::unhexWith0x(param0.AsString()));
    child_storage_key_ = common::Buffer(std::move(child_storage_key));

    auto &param1 = params[1];

    if (not param1.IsString()) {
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

    if (params.size() == 2) {
      return outcome::success();
    }

    if (not params[2].IsNil()) {
      // process at param
      if (not params[2].IsString()) {
        throw jsonrpc::InvalidParametersFault(
            "Parameter '[at]' must be a hex string representation of an "
            "encoded optional byte sequence");
      }
      OUTCOME_TRY(at_span, common::unhexWith0x(params[2].AsString()));
      OUTCOME_TRY(at, primitives::BlockHash::fromSpan(at_span));
      at_ = at;
    }
    return outcome::success();
  }

  outcome::result<std::vector<common::Buffer>> GetKeys::execute() {
    return api_->getKeys(child_storage_key_, prefix_, at_);
  }
}  // namespace kagome::api::child_state::request
