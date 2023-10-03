/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
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
                 const primitives::BabeConfiguration &config),
                (const, override));
  };

}  // namespace kagome::consensus::babe
