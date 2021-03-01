/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_API_RPC_REQUEST_METHODS
#define KAGOME_API_RPC_REQUEST_METHODS

#include <jsonrpc-lean/request.h>

#include "outcome/outcome.hpp"

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
    Methods(Methods const &) = delete;
    Methods &operator=(Methods const &) = delete;

    Methods(Methods &&) = default;
    Methods &operator=(Methods &&) = default;

    explicit Methods(std::shared_ptr<RpcApi> api) : api_(std::move(api)) {
      BOOST_ASSERT(api_);
    };
    ~Methods() = default;

    outcome::result<void> init(const jsonrpc::Request::Parameters &params);

    outcome::result<std::vector<std::string>> execute();

   private:
    std::shared_ptr<RpcApi> api_;
  };

}  // namespace kagome::api::rpc::request

#endif  // KAGOME_API_RPC_REQUEST_METHODS
