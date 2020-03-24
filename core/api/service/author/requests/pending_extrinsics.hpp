/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_API_SERVICE_EXTRINSIC_REQUEST_PENDING_EXTRINSICS_HPP
#define KAGOME_CORE_API_SERVICE_EXTRINSIC_REQUEST_PENDING_EXTRINSICS_HPP

#include "api/service/author/author_api.hpp"

#include <jsonrpc-lean/request.h>

#include "primitives/extrinsic.hpp"

namespace kagome::api::author::request {

class PendingExtrinsics final {
public:
		PendingExtrinsics(std::shared_ptr<AuthorApi> api) : api_(std::move(api)){};

		outcome::result<void> init(const jsonrpc::Request::Parameters &params);

		outcome::result<std::vector<primitives::Extrinsic>> execute();

private:
		std::shared_ptr<AuthorApi> api_;
};

}  // namespace kagome::api::author::request

#endif // KAGOME_CORE_API_SERVICE_EXTRINSIC_REQUEST_PENDING_EXTRINSICS_HPP
