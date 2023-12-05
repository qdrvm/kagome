/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "authorship/block_builder_factory.hpp"

#include <gmock/gmock.h>

namespace kagome::authorship {

  class BlockBuilderFactoryMock : public BlockBuilderFactory {
   public:
    MOCK_METHOD(outcome::result<std::unique_ptr<BlockBuilder>>,
                make,
                (const primitives::BlockInfo &,
                 primitives::Digest,
                 TrieChangesTrackerOpt),
                (const override));
  };

}  // namespace kagome::authorship
