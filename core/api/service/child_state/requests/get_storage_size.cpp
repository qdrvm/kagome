/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/service/child_state/requests/get_storage_size.hpp"

namespace kagome::api::child_state::request {

  outcome::result<void> GetStorageSize::init(
      const jsonrpc::Request::Parameters &params) {
    if (params.size() > 3 or params.size() < 2) {
      throw jsonrpc::InvalidParametersFault("Incorrect number of params");
    }
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
          "Parameter 'key' must be a hex string");
    }
    auto &&key_str = param1.AsString();
    OUTCOME_TRY(key, common::unhexWith0x(key_str));

    key_ = common::Buffer(std::move(key));

    if (params.size() == 3) {
      auto &param2 = params[2];
      if (param2.IsString()) {
        auto &&at_str = param2.AsString();
        OUTCOME_TRY(at_span, common::unhexWith0x(at_str));
        OUTCOME_TRY(at, primitives::BlockHash::fromSpan(at_span));
        at_.emplace(at);
      } else if (param2.IsNil()) {
        at_ = std::nullopt;
      } else {
        throw jsonrpc::InvalidParametersFault(
            "Parameter 'at' must be a hex string or null");
      }
    } else {
      at_.reset();
    }
    return outcome::success();
  }

  outcome::result<std::optional<uint64_t>> GetStorageSize::execute() {
    return api_->getStorageSize(child_storage_key_, key_, at_);
  }

}  // namespace kagome::api::child_state::request
