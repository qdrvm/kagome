/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_API_RPCAPI
#define KAGOME_API_RPCAPI

#include "api/service/api_service.hpp"
#include "outcome/outcome.hpp"

namespace kagome::api {

  class RpcApi {
   public:
    virtual ~RpcApi() = default;

    virtual outcome::result<std::vector<std::string>> methods() const = 0;
  };

}  // namespace kagome::api

#endif  // KAGOME_API_RPCAPI
