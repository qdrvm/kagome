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

#ifndef KAGOME_TEST_MOCK_CORE_RUNTIME_BLOCK_BUILDER_API_MOCK_HPP
#define KAGOME_TEST_MOCK_CORE_RUNTIME_BLOCK_BUILDER_API_MOCK_HPP

#include <gmock/gmock.h>
#include "runtime/block_builder.hpp"

namespace kagome::runtime {

  class BlockBuilderApiMock : public BlockBuilder {
   public:
    MOCK_METHOD1(apply_extrinsic,
                 outcome::result<primitives::ApplyResult>(
                     const primitives::Extrinsic &));
    MOCK_METHOD0(finalise_block, outcome::result<primitives::BlockHeader>());
    MOCK_METHOD1(inherent_extrinsics,
                 outcome::result<std::vector<primitives::Extrinsic>>(
                     const primitives::InherentData &));
    MOCK_METHOD2(check_inherents,
                 outcome::result<primitives::CheckInherentsResult>(
                     const primitives::Block &,
                     const primitives::InherentData &));
    MOCK_METHOD0(random_seed, outcome::result<common::Hash256>());
  };

}  // namespace kagome::runtime

#endif  // KAGOME_TEST_MOCK_CORE_RUNTIME_BLOCK_BUILDER_API_MOCK_HPP
