/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_API_SERVICE_STATE_REQUESTS_GET_KEYS_PAGED_HPP
#define KAGOME_CORE_API_SERVICE_STATE_REQUESTS_GET_KEYS_PAGED_HPP

#include <jsonrpc-lean/request.h>

#include <boost/optional.hpp>

#include "api/service/state/state_api.hpp"
#include "outcome/outcome.hpp"

namespace kagome::api::state::request {

  class GetKeysPaged final {
   public:
    GetKeysPaged(GetKeysPaged const &) = delete;
    GetKeysPaged &operator=(GetKeysPaged const &) = delete;

    GetKeysPaged(GetKeysPaged &&) = default;
    GetKeysPaged &operator=(GetKeysPaged &&) = default;

    explicit GetKeysPaged(std::shared_ptr<StateApi> api) : api_(std::move(api)){};
    ~GetKeysPaged() = default;

    outcome::result<void> init(const jsonrpc::Request::Parameters &params);

    outcome::result<std::vector<common::Buffer>> execute();

   private:
    std::shared_ptr<StateApi> api_;
    boost::optional<common::Buffer> prefix_{};
    uint32_t keys_amount_;
    boost::optional<common::Buffer> prev_key_{};
    boost::optional<primitives::BlockHash> at_{};
  };

}  // namespace kagome::api::state::request


#endif  // KAGOME_CORE_API_SERVICE_STATE_REQUESTS_GET_KEYS_PAGED_HPP
