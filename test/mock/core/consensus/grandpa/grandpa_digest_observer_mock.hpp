/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_grandpa_digest_observer_MOCK
#define KAGOME_grandpa_digest_observer_MOCK

#include "consensus/grandpa/grandpa_digest_observer.hpp"

#include <gmock/gmock.h>

namespace kagome::consensus::grandpa {
  struct GrandpaDigestObserverMock : public GrandpaDigestObserver {
    MOCK_METHOD(outcome::result<void>,
                onDigest,
                (const primitives::BlockContext &,
                 const primitives::GrandpaDigest &),
                (override));

    MOCK_METHOD(void, cancel, (const primitives::BlockInfo &), (override));
  };
}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_grandpa_digest_observer_MOCK
