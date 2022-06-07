/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_AUTHORITY_UPDATE_OBSERVER_MOCK
#define KAGOME_AUTHORITY_UPDATE_OBSERVER_MOCK

#include "consensus/authority/authority_update_observer.hpp"

#include <gmock/gmock.h>

namespace kagome::authority {
  struct AuthorityUpdateObserverMock : public AuthorityUpdateObserver {
    MOCK_METHOD(outcome::result<void>,
                onConsensus,
                (const primitives::BlockInfo &, const primitives::Consensus &),
                (override));

    MOCK_METHOD(void, cancel, (const primitives::BlockInfo &), (override));
  };
}  // namespace kagome::authority

#endif  // KAGOME_AUTHORITY_UPDATE_OBSERVER_MOCK
