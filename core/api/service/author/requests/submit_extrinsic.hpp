/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_API_SERVICE_EXTRINSIC_REQUEST_SUBMIT_EXTRINSIC_HPP
#define KAGOME_CORE_API_SERVICE_EXTRINSIC_REQUEST_SUBMIT_EXTRINSIC_HPP

#include "api/service/author/author_api.hpp"

#include <jsonrpc-lean/request.h>

#include "primitives/extrinsic.hpp"

namespace kagome::api::author::request {

  class SubmitExtrinsic final {
   public:
    SubmitExtrinsic(std::shared_ptr<AuthorApi> api) : api_(std::move(api)){};

    outcome::result<void> init(const jsonrpc::Request::Parameters &params);

    outcome::result<common::Hash256> execute();

   private:
    std::shared_ptr<AuthorApi> api_;
    primitives::Extrinsic extrinsic_;
  };

}  // namespace kagome::api::author::request

#endif // KAGOME_CORE_API_SERVICE_EXTRINSIC_REQUEST_SUBMIT_EXTRINSIC_HPP
