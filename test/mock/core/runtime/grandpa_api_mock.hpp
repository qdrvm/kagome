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
    MOCK_METHOD2(pending_change,
                 outcome::result<boost::optional<ScheduledChange>>(
                     primitives::BlockHash const &block, const Digest &digest));

    MOCK_METHOD2(forced_change,
                 outcome::result<boost::optional<ForcedChange>>(
                     primitives::BlockHash const &block, const Digest &digest));

    MOCK_METHOD1(
        authorities,
        outcome::result<AuthorityList>(const primitives::BlockId &block_id));
  };

}  // namespace kagome::runtime

#endif  // KAGOME_RUNTIME_GRANDPAAPIMOCK
