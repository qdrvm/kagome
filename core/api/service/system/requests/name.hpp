/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_API_SYSTEM_REQUEST_NAME
#define KAGOME_API_SYSTEM_REQUEST_NAME

#include <jsonrpc-lean/request.h>

#include "outcome/outcome.hpp"

namespace kagome::api {
  class SystemApi;
}

namespace kagome::api::system::request {

  /**
   * @brief Get the node's implementation name
   * @see https://github.com/w3f/PSPs/blob/psp-rpc-api/psp-002.md#system_name
   */
  class Name final {
   public:
    Name(const Name &) = delete;
    Name &operator=(const Name &) = delete;

    Name(Name &&) = default;
    Name &operator=(Name &&) = default;

    explicit Name(std::shared_ptr<SystemApi> api);
    ~Name() = default;

    outcome::result<void> init(const jsonrpc::Request::Parameters &params);

    outcome::result<std::string> execute();

   private:
    std::shared_ptr<SystemApi> api_;
  };

}  // namespace kagome::api::system::request

#endif  // KAGOME_API_SYSTEM_REQUEST_NAME
