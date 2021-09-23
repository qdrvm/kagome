/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/service/state/requests/get_storage.hpp"

namespace kagome::api::state::request {

  outcome::result<void> GetStorage::init(
      const jsonrpc::Request::Parameters &params) {
    if (params.size() > 2 or params.empty()) {
      throw jsonrpc::InvalidParametersFault("Incorrect number of params");
    }
    auto &param0 = params[0];
    if (not param0.IsString()) {
      throw jsonrpc::InvalidParametersFault(
          "Parameter 'key' must be a hex string");
    }
    auto &&key_str = param0.AsString();
    OUTCOME_TRY(key, common::unhexWith0x(key_str));

    key_ = common::Buffer(std::move(key));

    if (params.size() > 1) {
      auto &param1 = params[1];
      if (param1.IsString()) {
        auto &&at_str = param1.AsString();
        OUTCOME_TRY(at_span, common::unhexWith0x(at_str));
        OUTCOME_TRY(at, primitives::BlockHash::fromSpan(at_span));
        at_.reset(at);
      } else if (param1.IsNil()) {
        at_ = boost::none;
      } else {
        throw jsonrpc::InvalidParametersFault(
            "Parameter 'at' must be a hex string or null");
      }
    } else {
      at_.reset();
    }
    return outcome::success();
  }

  outcome::result<boost::optional<common::Buffer>> GetStorage::execute() {
    return at_ ? api_->getStorage(key_, at_.value()) : api_->getStorage(key_);
  }

}  // namespace kagome::api::state::request
