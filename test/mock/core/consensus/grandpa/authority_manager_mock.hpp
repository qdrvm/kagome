/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_GRANDPA_AUTHORITYMANAGERMOCK
#define KAGOME_CONSENSUS_GRANDPA_AUTHORITYMANAGERMOCK

#include "consensus/grandpa/authority_manager.hpp"

#include <gmock/gmock.h>

namespace kagome::consensus::grandpa {

  struct AuthorityManagerMock : public AuthorityManager {
    MOCK_METHOD(std::optional<std::shared_ptr<const primitives::AuthoritySet>>,
                authorities,
                (const primitives::BlockInfo &, IsBlockFinalized),
                (const, override));

    MOCK_METHOD(void,
                warp,
                (const primitives::BlockInfo &,
                 const primitives::BlockHeader &,
                 const primitives::AuthoritySet &),
                (override));
  };
}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CONSENSUS_GRANDPA_AUTHORITYMANAGERMOCK
