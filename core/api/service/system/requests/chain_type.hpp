/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_API_SERVICE_SYSTEM_REQUESTS_CHAIN_TYPE_HPP
#define KAGOME_CORE_API_SERVICE_SYSTEM_REQUESTS_CHAIN_TYPE_HPP

#include <jsonrpc-lean/request.h>

#include "outcome/outcome.hpp"

namespace kagome::api {
  class SystemApi;
}

namespace kagome::api::system::request {

  /**
   * @brief Get the chain's type. Given as a string identifier
   */
  class ChainType final {
   public:
    ChainType(const ChainType &) = delete;
    ChainType &operator=(const ChainType &) = delete;

    ChainType(ChainType &&) = default;
    ChainType &operator=(ChainType &&) = default;

    explicit ChainType(std::shared_ptr<SystemApi> api);
    ~ChainType() = default;

    outcome::result<void> init(const jsonrpc::Request::Parameters &params);

    outcome::result<std::string> execute();

   private:
    std::shared_ptr<SystemApi> api_;
  };

}  // namespace kagome::api::system::request

#endif  // KAGOME_CORE_API_SERVICE_SYSTEM_REQUESTS_CHAIN_TYPE_HPP
