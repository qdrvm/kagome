/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef KAGOME_TEST_MOCK_CORE_CONSENSUS_VALIDATION_BLOCK_VALIDATOR_MOCK_HPP
#define KAGOME_TEST_MOCK_CORE_CONSENSUS_VALIDATION_BLOCK_VALIDATOR_MOCK_HPP

#include "consensus/validation/babe_block_validator.hpp"

#include <gmock/gmock.h>

namespace kagome::consensus {

  class BlockValidatorMock : public BlockValidator {
   public:
    MOCK_CONST_METHOD4(
        validateBlock,
        outcome::result<void>(const primitives::Block &block,
                              const primitives::AuthorityId &authority_id,
                              const Threshold &threshold,
                              const Randomness &randomness));

    MOCK_CONST_METHOD4(
        validateHeader,
        outcome::result<void>(const primitives::BlockHeader &header,
                              const primitives::AuthorityId &authority_id,
                              const Threshold &threshold,
                              const Randomness &randomness));
  };

}  // namespace kagome::consensus

#endif  // KAGOME_TEST_MOCK_CORE_CONSENSUS_VALIDATION_BLOCK_VALIDATOR_MOCK_HPP
