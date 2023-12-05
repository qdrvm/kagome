/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/grandpa/vote_crypto_provider.hpp"

#include <gmock/gmock.h>

namespace kagome::consensus::grandpa {

  class VoteCryptoProviderMock : public VoteCryptoProvider {
   public:
    MOCK_METHOD(bool,
                verifyPrimaryPropose,
                (const SignedMessage &primary_propose),
                (const, override));

    MOCK_METHOD(bool,
                verifyPrevote,
                (const SignedMessage &prevote),
                (const, override));

    MOCK_METHOD(bool,
                verifyPrecommit,
                (const SignedMessage &precommit),
                (const, override));

    MOCK_METHOD(std::optional<SignedMessage>,
                signPrimaryPropose,
                (const PrimaryPropose &primary_propose),
                (const, override));

    MOCK_METHOD(std::optional<SignedMessage>,
                signPrevote,
                (const Prevote &prevote),
                (const, override));

    MOCK_METHOD(std::optional<SignedMessage>,
                signPrecommit,
                (const Precommit &precommit),
                (const, override));
  };

}  // namespace kagome::consensus::grandpa
