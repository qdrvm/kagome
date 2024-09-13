/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "runtime/runtime_api/beefy.hpp"

#include <gmock/gmock.h>

namespace kagome::runtime {
  struct BeefyApiMock : BeefyApi {
    MOCK_METHOD(outcome::result<std::optional<primitives::BlockNumber>>,
                genesis,
                (const primitives::BlockHash &),
                (override));

    MOCK_METHOD(outcome::result<std::optional<consensus::beefy::ValidatorSet>>,
                validatorSet,
                (const primitives::BlockHash &),
                (override));

    MOCK_METHOD(outcome::result<void>,
                submit_report_double_voting_unsigned_extrinsic,
                (const primitives::BlockHash &,
                 const consensus::beefy::DoubleVotingProof &,
                 const primitives::OpaqueKeyOwnershipProof &),
                (const, override));

    MOCK_METHOD(
        outcome::result<std::optional<primitives::OpaqueKeyOwnershipProof>>,
        generate_key_ownership_proof,
        (const primitives::BlockHash &,
         consensus::beefy::AuthoritySetId,
         const crypto::EcdsaPublicKey &),
        (const, override));
  };
}  // namespace kagome::runtime
