/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_MOCK_CORE_RUNTIME_BLOCK_BUILDER_API_MOCK_HPP
#define KAGOME_TEST_MOCK_CORE_RUNTIME_BLOCK_BUILDER_API_MOCK_HPP

#include <gmock/gmock.h>
#include "runtime/block_builder.hpp"

namespace kagome::runtime {

  class BlockBuilderApiMock : public BlockBuilder {
   public:
    MOCK_METHOD1(apply_extrinsic,
                 outcome::result<primitives::ApplyExtrinsicResult>(
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
