/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gmock/gmock.h>

#include "parachain/availability/bitfield/signer.hpp"

namespace kagome::parachain {

  class BitfieldSignerMock : public IBitfieldSigner {
   public:
    MOCK_METHOD(void, start, (), (override));

    MOCK_METHOD(outcome::result<void>,
                sign,
                (const ValidatorSigner &, const Candidates &),
                (override));

    MOCK_METHOD(void, setBroadcastCallback, (BroadcastCallback &&), (override));
  };

}  // namespace kagome::parachain
