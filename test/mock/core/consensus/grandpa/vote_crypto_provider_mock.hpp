/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_MOCK_CORE_CONSENSUS_GRANDPA_VOTE_CRYPTO_PROVIDER_MOCK_HPP
#define KAGOME_TEST_MOCK_CORE_CONSENSUS_GRANDPA_VOTE_CRYPTO_PROVIDER_MOCK_HPP

#include "consensus/grandpa/vote_crypto_provider.hpp"

#include <gmock/gmock.h>

namespace kagome::consensus::grandpa {

  class VoteCryptoProviderMock : public VoteCryptoProvider {
   public:
    MOCK_CONST_METHOD1(verifyPrimaryPropose,
                       bool(const SignedPrimaryPropose &primary_propose));
    MOCK_CONST_METHOD1(verifyPrevote, bool(const SignedPrevote &prevote));
    MOCK_CONST_METHOD1(verifyPrecommit, bool(const SignedPrecommit &precommit));

    MOCK_CONST_METHOD1(
        signPrimaryPropose,
        SignedPrimaryPropose(const PrimaryPropose &primary_propose));
    MOCK_CONST_METHOD1(signPrevote, SignedPrevote(const Prevote &prevote));
    MOCK_CONST_METHOD1(signPrecommit,
                       SignedPrecommit(const Precommit &precommit));
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_TEST_MOCK_CORE_CONSENSUS_GRANDPA_VOTE_CRYPTO_PROVIDER_MOCK_HPP
