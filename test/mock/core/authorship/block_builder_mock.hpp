/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "authorship/block_builder.hpp"

#include <gmock/gmock.h>

namespace kagome::authorship {

  class BlockBuilderMock : public BlockBuilder {
   public:
    MOCK_METHOD(outcome::result<std::vector<primitives::Extrinsic>>,
                getInherentExtrinsics,
                (const primitives::InherentData &data),
                (const, override));

    MOCK_METHOD(outcome::result<primitives::ExtrinsicIndex>,
                pushExtrinsic,
                (const primitives::Extrinsic &extrinsic),
                (override));

    MOCK_METHOD(outcome::result<primitives::Block>,
                bake,
                (),
                (const, override));

    MOCK_METHOD(size_t, estimateBlockSize, (), (const, override));
  };

}  // namespace kagome::authorship
