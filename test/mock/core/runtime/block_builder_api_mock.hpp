/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gmock/gmock.h>
#include "runtime/runtime_api/block_builder.hpp"

namespace kagome::runtime {

  class BlockBuilderApiMock : public BlockBuilder {
   public:
    MOCK_METHOD(outcome::result<primitives::ApplyExtrinsicResult>,
                apply_extrinsic,
                (RuntimeContext &, const primitives::Extrinsic &),
                (override));

    MOCK_METHOD(outcome::result<primitives::BlockHeader>,
                finalize_block,
                (RuntimeContext &),
                (override));

    MOCK_METHOD(outcome::result<std::vector<primitives::Extrinsic>>,
                inherent_extrinsics,
                (RuntimeContext &, const primitives::InherentData &),
                (override));

    MOCK_METHOD(outcome::result<primitives::CheckInherentsResult>,
                check_inherents,
                (const primitives::Block &, const primitives::InherentData &),
                (override));

    MOCK_METHOD(outcome::result<common::Hash256>,
                random_seed,
                (const storage::trie::RootHash &storage_hash),
                (override));
  };

}  // namespace kagome::runtime
