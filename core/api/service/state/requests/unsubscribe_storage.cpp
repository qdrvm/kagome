/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/service/state/requests/unsubscribe_storage.hpp"

namespace kagome::api::state::request {

  outcome::result<void> UnsubscribeStorage::init(
      const jsonrpc::Request::Parameters &params) {
    if (params.size() != 1) {
      throw jsonrpc::InvalidParametersFault(
          "unsubscribeStorage takes one parameter");
    }

    auto &id = params[0];
    if (!id.IsInteger32()) {
      throw jsonrpc::InvalidParametersFault(
          "Parameter 'params' must be an integer value of subscriber ID");
    }

    subscriber_id_.emplace_back(id.AsInteger32());
    return outcome::success();
  }

  outcome::result<bool> UnsubscribeStorage::execute() {
    return api_->unsubscribeStorage(subscriber_id_);
  }

}  // namespace kagome::api::state::request
