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
   * @brief Get the node implementation's version. Should be a semver string
   * @see https://github.com/w3f/PSPs/blob/psp-rpc-api/psp-002.md#system_version
   */
  class Version final {
   public:
    Version(const Version &) = delete;
    Version &operator=(const Version &) = delete;

    Version(Version &&) = default;
    Version &operator=(Version &&) = default;

    explicit Version(std::shared_ptr<SystemApi> api);
    ~Version() = default;

    outcome::result<void> init(const jsonrpc::Request::Parameters &params);

    outcome::result<std::string> execute();

   private:
    std::shared_ptr<SystemApi> api_;
  };

}  // namespace kagome::api::system::request
