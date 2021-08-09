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
    MOCK_METHOD3(
        apply_extrinsic,
        outcome::result<PersistentResult<primitives::ApplyExtrinsicResult>>(
            const primitives::BlockInfo &block,
            storage::trie::RootHash const &storage_hash,
            const primitives::Extrinsic &));
    MOCK_METHOD2(finalize_block,
                 outcome::result<primitives::BlockHeader>(
                     const primitives::BlockInfo &block,
                     storage::trie::RootHash const &storage_hash));
    MOCK_METHOD3(inherent_extrinsics,
                 outcome::result<std::vector<primitives::Extrinsic>>(
                     const primitives::BlockInfo &block,
                     storage::trie::RootHash const &storage_hash,
                     const primitives::InherentData &));
    MOCK_METHOD2(check_inherents,
                 outcome::result<primitives::CheckInherentsResult>(
                     const primitives::Block &,
                     const primitives::InherentData &));
    MOCK_METHOD1(random_seed,
                 outcome::result<common::Hash256>(
                     storage::trie::RootHash const &storage_hash));
  };

}  // namespace kagome::runtime

#endif  // KAGOME_TEST_MOCK_CORE_RUNTIME_BLOCK_BUILDER_API_MOCK_HPP
