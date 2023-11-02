/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "runtime/runtime_api/grandpa_api.hpp"

#include <gmock/gmock.h>

namespace kagome::runtime {

  class GrandpaApiMock : public GrandpaApi {
   public:
    MOCK_METHOD(outcome::result<AuthorityList>,
                authorities,
                (const primitives::BlockHash &),
                (override));

    MOCK_METHOD(outcome::result<primitives::AuthoritySetId>,
                current_set_id,
                (const primitives::BlockHash &block),
                (override));
  };

}  // namespace kagome::runtime
