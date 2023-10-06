/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_API_SERVICE_STATE_REQUESTS_CALL_HPP
#define KAGOME_CORE_API_SERVICE_STATE_REQUESTS_CALL_HPP

#include <jsonrpc-lean/request.h>

#include <optional>

#include "api/service/state/state_api.hpp"
#include "common/buffer.hpp"
#include "outcome/outcome.hpp"

namespace kagome::api::state::request {

  /**
   * Request processor for state_call RPC:
   * https://polkadot.js.org/docs/substrate/rpc/#callmethod-text-data-bytes-at-blockhash-bytes
   */
  class Call final {
   public:
    Call(const Call &) = delete;
    Call &operator=(const Call &) = delete;

    Call(Call &&) = default;
    Call &operator=(Call &&) = default;

    explicit Call(std::shared_ptr<StateApi> api);
    ~Call() = default;

    outcome::result<void> init(const jsonrpc::Request::Parameters &params);

    outcome::result<common::Buffer> execute();

   private:
    std::shared_ptr<StateApi> api_;
    std::string method_;
    common::Buffer data_;
    std::optional<primitives::BlockHash> at_;
  };

}  // namespace kagome::api::state::request

#endif  // KAGOME_CORE_API_SERVICE_STATE_REQUESTS_CALL_HPP
