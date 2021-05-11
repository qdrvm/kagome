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
    MOCK_METHOD2(
        authorities,
        outcome::result<std::shared_ptr<const primitives::AuthorityList>>(
            const primitives::BlockInfo &, bool));

    MOCK_METHOD3(applyScheduledChange,
                 outcome::result<void>(const primitives::BlockInfo &,
                                       const primitives::AuthorityList &,
                                       primitives::BlockNumber));

    MOCK_METHOD3(applyForcedChange,
                 outcome::result<void>(const primitives::BlockInfo &,
                                       const primitives::AuthorityList &,
                                       primitives::BlockNumber));

    MOCK_METHOD2(applyOnDisabled,
                 outcome::result<void>(const primitives::BlockInfo &,
                                       uint64_t));

    MOCK_METHOD2(applyPause,
                 outcome::result<void>(const primitives::BlockInfo &,
                                       primitives::BlockNumber));

    MOCK_METHOD2(applyResume,
                 outcome::result<void>(const primitives::BlockInfo &,
                                       primitives::BlockNumber));

    MOCK_METHOD1(prune,
                 outcome::result<void>(const primitives::BlockInfo &block));
  };
}  // namespace kagome::authority

#endif  // KAGOME_AUTHORITY_AUTHORITYMANAGERMOCK
