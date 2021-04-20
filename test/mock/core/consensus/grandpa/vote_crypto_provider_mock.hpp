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
                       bool(const SignedMessage &primary_propose));
    MOCK_CONST_METHOD1(verifyPrevote, bool(const SignedMessage &prevote));
    MOCK_CONST_METHOD1(verifyPrecommit, bool(const SignedMessage &precommit));

    MOCK_CONST_METHOD1(
        signPrimaryPropose,
        boost::optional<SignedMessage>(const PrimaryPropose &primary_propose));
    MOCK_CONST_METHOD1(signPrevote,
                       boost::optional<SignedMessage>(const Prevote &prevote));
    MOCK_CONST_METHOD1(
        signPrecommit,
        boost::optional<SignedMessage>(const Precommit &precommit));
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_TEST_MOCK_CORE_CONSENSUS_GRANDPA_VOTE_CRYPTO_PROVIDER_MOCK_HPP
