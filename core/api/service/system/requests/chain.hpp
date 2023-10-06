/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_API_SYSTEM_REQUEST_CHAIN
#define KAGOME_API_SYSTEM_REQUEST_CHAIN

#include <jsonrpc-lean/request.h>

#include "outcome/outcome.hpp"

namespace kagome::api {
  class SystemApi;
}

namespace kagome::api::system::request {

  /**
   * @brief Get the chain's type. Given as a string identifier
   * @see
   *  https://github.com/w3f/PSPs/blob/psp-rpc-api/psp-002.md#system_chain
   */
  class Chain final {
   public:
    Chain(const Chain &) = delete;
    Chain &operator=(const Chain &) = delete;

    Chain(Chain &&) = default;
    Chain &operator=(Chain &&) = default;

    explicit Chain(std::shared_ptr<SystemApi> api);
    ~Chain() = default;

    outcome::result<void> init(const jsonrpc::Request::Parameters &params);

    outcome::result<std::string> execute();

   private:
    std::shared_ptr<SystemApi> api_;
  };

}  // namespace kagome::api::system::request

#endif  // KAGOME_API_SYSTEM_REQUEST_CHAIN
