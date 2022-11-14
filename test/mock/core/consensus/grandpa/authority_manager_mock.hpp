/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_GRANDPA_AUTHORITYMANAGERMOCK
#define KAGOME_CONSENSUS_GRANDPA_AUTHORITYMANAGERMOCK

#include "consensus/grandpa/authority_manager.hpp"
#include "grandpa_digest_observer_mock.hpp"

#include <gmock/gmock.h>

namespace kagome::consensus::grandpa {

  struct AuthorityManagerMock : public AuthorityManager {
    MOCK_METHOD(primitives::BlockInfo, base, (), (const, override));

    MOCK_METHOD(std::optional<std::shared_ptr<const primitives::AuthoritySet>>,
                authorities,
                (const primitives::BlockInfo &, IsBlockFinalized),
                (const, override));

    MOCK_METHOD(outcome::result<void>,
                applyScheduledChange,
                (const primitives::BlockInfo &,
                 const primitives::AuthorityList &,
                 primitives::BlockNumber),
                (override));

    MOCK_METHOD(outcome::result<void>,
                applyForcedChange,
                (const primitives::BlockInfo &,
                 const primitives::AuthorityList &,
                 primitives::BlockNumber,
                 size_t),
                (override));

    MOCK_METHOD(outcome::result<void>,
                applyOnDisabled,
                (const primitives::BlockInfo &, uint64_t),
                (override));

    MOCK_METHOD(outcome::result<void>,
                applyPause,
                (const primitives::BlockInfo &, primitives::BlockNumber),
                (override));

    MOCK_METHOD(outcome::result<void>,
                applyResume,
                (const primitives::BlockInfo &, primitives::BlockNumber),
                (override));
  };
}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CONSENSUS_GRANDPA_AUTHORITYMANAGERMOCK
