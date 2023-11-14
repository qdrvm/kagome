/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/validation/babe_block_validator.hpp"

#include <gmock/gmock.h>

namespace kagome::consensus::babe {

  class BlockValidatorMock : public BlockValidator {
   public:
    MOCK_METHOD(outcome::result<void>,
                validateBlock,
                (const primitives::Block &,
                 const primitives::AuthorityId &,
                 const Threshold &,
                 const Randomness &),
                (const));

    MOCK_METHOD(outcome::result<void>,
                validateHeader,
                (const primitives::BlockHeader &,
                 const EpochNumber,
                 const primitives::AuthorityId &,
                 const Threshold &,
                 const consensus::babe::BabeConfiguration &),
                (const, override));
  };

}  // namespace kagome::consensus::babe
