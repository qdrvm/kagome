/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gmock/gmock.h>

#include "outcome/outcome.hpp"

namespace kagome::api {

  /**
   * @brief MPV implementation of possible API for using in tests instead real
   * powerful API
   */
  class ApiStub {
   public:
    outcome::result<int64_t> echo(int64_t payload) {
      return payload;
    }
  };

}  // namespace kagome::api
