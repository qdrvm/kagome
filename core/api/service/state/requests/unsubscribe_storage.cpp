/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/service/state/requests/unsubscribe_storage.hpp"

namespace kagome::api::state::request {

  outcome::result<void> UnsubscribeStorage::init(
      const jsonrpc::Request::Parameters &params) {
    if (params.size() > 1 or params.empty()) {
      throw jsonrpc::InvalidParametersFault("Incorrect number of params");
    }

    auto &ids = params[0];
    if (!ids.IsArray()) {
      throw jsonrpc::InvalidParametersFault(
          "Parameter 'params' must be a UINT array of the subscription ids");
    }

    subscriber_id_.reserve(ids.AsArray().size());
    for (auto &id : ids.AsArray()) {
      if (!id.IsInteger32())
        throw jsonrpc::InvalidParametersFault( "Parameter 'params' must be an integer value of subscriber ID");

      subscriber_id_.emplace_back(id.AsInteger32());
    }
    return outcome::success();
  }

  outcome::result<void> UnsubscribeStorage::execute() {
    return api_->unsubscribeStorage(subscriber_id_);
  }

}  // namespace kagome::api::state::request
