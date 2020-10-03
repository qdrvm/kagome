/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_API_REQUEST_GET_METADATA
#define KAGOME_API_REQUEST_GET_METADATA

#include <jsonrpc-lean/request.h>

#include <boost/optional.hpp>
#include <vector>

#include "api/service/state/state_api.hpp"
#include "common/buffer.hpp"
#include "outcome/outcome.hpp"
#include "primitives/block_id.hpp"
#include "api/service/chain/requests/base_request.hpp"

namespace kagome::api::state::request {

  struct GetMetadata final : chain::request::RequestType<std::vector<uint8_t>> {
   public:
    using ReturnType = std::vector<uint8_t>;

    GetMetadata(GetMetadata const &) = delete;
    GetMetadata &operator=(GetMetadata const &) = delete;

    GetMetadata(GetMetadata &&) = default;
    GetMetadata &operator=(GetMetadata &&) = default;

    explicit GetMetadata(std::shared_ptr<StateApi> api)
        : api_(std::move(api)){};
    ~GetMetadata() = default;

    outcome::result<ReturnType> execute() {
      return api_->getMetadata();
    }

   private:
    std::shared_ptr<StateApi> api_;
  };

}  // namespace kagome::api::state::request

#endif  // KAGOME_API_REQUEST_GET_METADATA
