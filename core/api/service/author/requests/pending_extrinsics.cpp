/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/service/author/requests/pending_extrinsics.hpp"

namespace kagome::api::author::request {

  outcome::result<void> PendingExtrinsics::init(
      const jsonrpc::Request::Parameters &params) {
    return outcome::success();
  }

  outcome::result<std::vector<primitives::Extrinsic>>
  PendingExtrinsics::execute() {
    return api_->pendingExtrinsics();
  }

}  // namespace kagome::api::author::request
