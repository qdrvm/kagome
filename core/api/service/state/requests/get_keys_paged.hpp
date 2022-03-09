/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_API_SERVICE_STATE_REQUESTS_GET_KEYS_PAGED_HPP
#define KAGOME_CORE_API_SERVICE_STATE_REQUESTS_GET_KEYS_PAGED_HPP

#include <jsonrpc-lean/request.h>

#include <optional>

#include "api/service/state/state_api.hpp"
#include "outcome/outcome.hpp"

namespace kagome::api::state::request {

  /**
   * Request processor for state_GetKeysPaged RPC:
   * https://github.com/w3f/PSPs/blob/psp-rpc-api/psp-002.md#state_getkeyspaged
   */
  class GetKeysPaged final {
   public:
    GetKeysPaged(GetKeysPaged const &) = delete;
    GetKeysPaged &operator=(GetKeysPaged const &) = delete;

    GetKeysPaged(GetKeysPaged &&) = default;
    GetKeysPaged &operator=(GetKeysPaged &&) = default;

    explicit GetKeysPaged(std::shared_ptr<StateApi> api)
        : api_(std::move(api)) {
      BOOST_ASSERT(api_);
    };
    ~GetKeysPaged() = default;

    outcome::result<void> init(const jsonrpc::Request::Parameters &params);

    outcome::result<std::vector<common::Buffer>> execute();

   private:
    std::shared_ptr<StateApi> api_;
    std::optional<common::Buffer> prefix_;
    uint32_t keys_amount_{};
    std::optional<common::BufferView> prev_key_;
    std::optional<primitives::BlockHash> at_;
  };

}  // namespace kagome::api::state::request

#endif  // KAGOME_CORE_API_SERVICE_STATE_REQUESTS_GET_KEYS_PAGED_HPP
