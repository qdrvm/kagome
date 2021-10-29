/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_MOCK_CORE_CONSENSUS_VALIDATION_BLOCK_VALIDATOR_MOCK_HPP
#define KAGOME_TEST_MOCK_CORE_CONSENSUS_VALIDATION_BLOCK_VALIDATOR_MOCK_HPP

#include "consensus/validation/babe_block_validator.hpp"

#include <gmock/gmock.h>

namespace kagome::consensus {

  class BlockValidatorMock : public BlockValidator {
   public:
    MOCK_METHOD(outcome::result<void>,
                validateBlock,
                (const primitives::Block &block,
                 const primitives::AuthorityId &authority_id,
                 const Threshold &threshold,
                 const Randomness &randomness),
                (const));

    MOCK_METHOD(outcome::result<void>,
                validateHeader,
                (const primitives::BlockHeader &header,
                 const EpochNumber epoch_number,
                 const primitives::AuthorityId &authority_id,
                 const Threshold &threshold,
                 const Randomness &randomness),
                (const, override));
  };

}  // namespace kagome::consensus

#endif  // KAGOME_TEST_MOCK_CORE_CONSENSUS_VALIDATION_BLOCK_VALIDATOR_MOCK_HPP
