/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_API_SYSTEM_REQUEST_PROPERTIES
#define KAGOME_API_SYSTEM_REQUEST_PROPERTIES

#include <jsonrpc-lean/request.h>

#include "api/service/system/system_api.hpp"
#include "outcome/outcome.hpp"

namespace kagome::api::system::request {

  /**
   * @brief Get a custom set of properties as a JSON object, defined in the
   *  chain spec
   * @see
   *  https://github.com/w3f/PSPs/blob/psp-rpc-api/psp-002.md#system_properties
   */
  class Properties final {
   public:
    Properties(Properties const &) = delete;
    Properties &operator=(Properties const &) = delete;

    Properties(Properties &&) = default;
    Properties &operator=(Properties &&) = default;

    explicit Properties(std::shared_ptr<SystemApi> api)
        : api_(std::move(api)){};
    ~Properties() = default;

    outcome::result<void> init(const jsonrpc::Request::Parameters &params);

    outcome::result<std::map<std::string, std::string>> execute();

   private:
    std::shared_ptr<SystemApi> api_;
  };

}  // namespace kagome::api::system::request

#endif  // KAGOME_API_SYSTEM_REQUEST_PROPERTIES
