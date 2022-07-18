/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_RUNTIME_GRANDPAAPIMOCK
#define KAGOME_RUNTIME_GRANDPAAPIMOCK

#include "runtime/runtime_api/grandpa_api.hpp"

#include <gmock/gmock.h>

namespace kagome::runtime {

  class GrandpaApiMock : public GrandpaApi {
   public:
    MOCK_METHOD(outcome::result<std::optional<ScheduledChange>>,
                pending_change,
                (primitives::BlockHash const &block, const Digest &digest),
                (override));

    MOCK_METHOD(outcome::result<std::optional<ForcedChange>>,
                forced_change,
                (primitives::BlockHash const &block, const Digest &digest),
                (override));

    MOCK_METHOD(outcome::result<AuthorityList>,
                authorities,
                (const primitives::BlockId &block_id),
                (override));

    MOCK_METHOD(outcome::result<primitives::AuthoritySetId>,
                current_set_id,
                (const primitives::BlockHash &block));
  };

}  // namespace kagome::runtime

#endif  // KAGOME_RUNTIME_GRANDPAAPIMOCK
