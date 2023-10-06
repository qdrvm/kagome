/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_API_SERVICE_CHILD_STATE_REQUESTS_GET_KEYS_HPP
#define KAGOME_CORE_API_SERVICE_CHILD_STATE_REQUESTS_GET_KEYS_HPP

#include <optional>

#include <jsonrpc-lean/request.h>

#include "api/service/child_state/child_state_api.hpp"
#include "outcome/outcome.hpp"

namespace kagome::api::child_state::request {

  /**
   * Request processor for childstate_GetKeys RPC:
   * https://github.com/w3f/PSPs/blob/master/PSPs/drafts/psp-6.md#1121-childstate_getkeys
   */
  class GetKeys final {
   public:
    GetKeys(const GetKeys &) = delete;
    GetKeys &operator=(const GetKeys &) = delete;

    GetKeys(GetKeys &&) = default;
    GetKeys &operator=(GetKeys &&) = default;

    explicit GetKeys(std::shared_ptr<ChildStateApi> api)
        : api_(std::move(api)) {
      BOOST_ASSERT(api_);
    };
    ~GetKeys() = default;

    outcome::result<void> init(const jsonrpc::Request::Parameters &params);

    outcome::result<std::vector<common::Buffer>> execute();

   private:
    std::shared_ptr<ChildStateApi> api_;
    common::Buffer child_storage_key_;
    std::optional<common::Buffer> prefix_;
    std::optional<primitives::BlockHash> at_;
  };

}  // namespace kagome::api::child_state::request

#endif  // KAGOME_CORE_API_SERVICE_CHILD_STATE_REQUESTS_GET_KEYS_HPP
