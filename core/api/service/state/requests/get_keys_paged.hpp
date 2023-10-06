/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

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
    GetKeysPaged(const GetKeysPaged &) = delete;
    GetKeysPaged &operator=(const GetKeysPaged &) = delete;

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
    std::optional<common::Buffer> prev_key_;
    std::optional<primitives::BlockHash> at_;
  };

}  // namespace kagome::api::state::request
