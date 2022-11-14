/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_AUTHORITY_AUTHORITYMANAGERMOCK
#define KAGOME_AUTHORITY_AUTHORITYMANAGERMOCK

#include "consensus/authority/authority_manager.hpp"
#include "mock/core/consensus/authority/authority_update_observer_mock.hpp"

#include <gmock/gmock.h>

namespace kagome::authority {

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

    MOCK_METHOD(void, prune, (const primitives::BlockInfo &block), (override));

    MOCK_METHOD(outcome::result<void>,
                recalculateStoredState,
                (primitives::BlockNumber last_finalized_number));

    MOCK_METHOD(outcome::result<bool>,
                overwriteStoredRootId,
                (primitives::AuthoritySetId id),
                (override));
  };
}  // namespace kagome::authority

#endif  // KAGOME_AUTHORITY_AUTHORITYMANAGERMOCK
