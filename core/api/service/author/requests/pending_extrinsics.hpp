/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "api/service/base_request.hpp"
#include "primitives/extrinsic.hpp"

#include "api/service/author/author_api.hpp"

namespace kagome::api::author::request {

  /**
   * @brief Returns all pending extrinsics, potentially grouped by sender.
   * @see
   * https://github.com/w3f/PSPs/blob/psp-rpc-api/psp-002.md#author_pendingextrinsics
   */
  class PendingExtrinsics final
      : public details::RequestType<std::vector<primitives::Extrinsic>> {
   public:
    explicit PendingExtrinsics(std::shared_ptr<AuthorApi> api)
        : api_(std::move(api)){};

    outcome::result<Return> execute() {
      return api_->pendingExtrinsics();
    }

   private:
    std::shared_ptr<AuthorApi> api_;
  };

}  // namespace kagome::api::author::request
