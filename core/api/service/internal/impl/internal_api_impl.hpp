/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "api/service/internal/internal_api.hpp"

namespace kagome::api {

  class InternalApiImpl : public InternalApi {
   public:
    outcome::result<void> setLogLevel(const std::string &group,
                                      const std::string &level) override;
  };
}  // namespace kagome::api
