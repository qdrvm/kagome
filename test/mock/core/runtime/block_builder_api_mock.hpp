/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_MOCK_CORE_RUNTIME_BLOCK_BUILDER_API_MOCK_HPP
#define KAGOME_TEST_MOCK_CORE_RUNTIME_BLOCK_BUILDER_API_MOCK_HPP

#include <gmock/gmock.h>
#include "runtime/runtime_api/block_builder.hpp"

namespace kagome::runtime {

  class BlockBuilderApiMock : public BlockBuilder {
   public:
    MOCK_METHOD(outcome::result<primitives::ApplyExtrinsicResult>,
                apply_extrinsic,
                (RuntimeEnvironment &, const primitives::Extrinsic &),
                (override));

    MOCK_METHOD(outcome::result<primitives::BlockHeader>,
                finalize_block,
                (RuntimeEnvironment &),
                (override));

    MOCK_METHOD(outcome::result<std::vector<primitives::Extrinsic>>,
                inherent_extrinsics,
                (RuntimeEnvironment &, const primitives::InherentData &),
                (override));

    MOCK_METHOD(outcome::result<primitives::CheckInherentsResult>,
                check_inherents,
                (const primitives::Block &, const primitives::InherentData &),
                (override));

    MOCK_METHOD(outcome::result<common::Hash256>,
                random_seed,
                (storage::trie::RootHash const &storage_hash),
                (override));
  };

}  // namespace kagome::runtime

#endif  // KAGOME_TEST_MOCK_CORE_RUNTIME_BLOCK_BUILDER_API_MOCK_HPP
