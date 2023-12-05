/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <jsonrpc-lean/request.h>

#include "outcome/outcome.hpp"

namespace kagome::api {
  class SystemApi;
}

namespace kagome::api::system::request {

  /**
   * @brief Return health status of the node
   * @see https://github.com/w3f/PSPs/blob/psp-rpc-api/psp-002.md#system_health
   */
  class Health final {
   public:
    Health(const Health &) = delete;
    Health &operator=(const Health &) = delete;

    Health(Health &&) = default;
    Health &operator=(Health &&) = default;

    explicit Health(std::shared_ptr<SystemApi> api);
    ~Health() = default;

    outcome::result<void> init(const jsonrpc::Request::Parameters &params);

    outcome::result<jsonrpc::Value::Struct> execute();

   private:
    std::shared_ptr<SystemApi> api_;
  };

}  // namespace kagome::api::system::request
