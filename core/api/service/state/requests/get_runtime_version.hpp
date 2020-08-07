/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_API_REQUEST_GET_VERSION
#define KAGOME_API_REQUEST_GET_VERSION

#include <jsonrpc-lean/request.h>

#include "api/service/state/state_api.hpp"
#include "common/buffer.hpp"
#include "outcome/outcome.hpp"
#include "primitives/block_id.hpp"

namespace kagome::api::state::request {

  class GetRuntimeVersion final {
   public:
    GetRuntimeVersion(const GetRuntimeVersion &) = delete;
    GetRuntimeVersion &operator=(const GetRuntimeVersion &) = delete;

    GetRuntimeVersion(GetRuntimeVersion &&) = default;
    GetRuntimeVersion &operator=(GetRuntimeVersion &&) = default;

    explicit GetRuntimeVersion(std::shared_ptr<StateApi> api);
    ~GetRuntimeVersion() = default;

    outcome::result<void> init(jsonrpc::Request::Parameters const &params);
    outcome::result<primitives::Version> execute();

   private:
    std::shared_ptr<StateApi> api_;
    boost::optional<primitives::BlockHash> at_;
  };

}  // namespace kagome::api::state::request

#endif  // KAGOME_API_REQUEST_GET_VERSION
