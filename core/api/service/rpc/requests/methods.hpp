/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <jsonrpc-lean/request.h>
#include <boost/assert.hpp>

#include "outcome/outcome.hpp"
#include "primitives/rpc_methods.hpp"

namespace kagome::api {
  class RpcApi;
}

namespace kagome::api::rpc::request {

  /**
   * Request processor for RPC method 'rpc_methods'
   * This method returns list of supported RPC methods
   */
  class Methods final {
   public:
    Methods(const Methods &) = delete;
    Methods &operator=(const Methods &) = delete;

    Methods(Methods &&) = default;
    Methods &operator=(Methods &&) = default;

    explicit Methods(std::shared_ptr<RpcApi> api) : api_(std::move(api)) {
      BOOST_ASSERT(api_);
    };
    ~Methods() = default;

    outcome::result<void> init(const jsonrpc::Request::Parameters &params);

    outcome::result<primitives::RpcMethods> execute();

   private:
    std::shared_ptr<RpcApi> api_;
  };

}  // namespace kagome::api::rpc::request
