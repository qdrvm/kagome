/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "runtime/runtime_api/core.hpp"

#include <gmock/gmock.h>

namespace kagome::runtime {

  class CoreMock : public Core, public RestrictedCore {
   public:
    MOCK_METHOD(outcome::result<primitives::Version>, version, (), (override));

    MOCK_METHOD(outcome::result<primitives::Version>,
                version,
                (const primitives::BlockHash &block),
                (override));

    MOCK_METHOD(outcome::result<void>,
                execute_block,
                (const primitives::Block &, TrieChangesTrackerOpt),
                (override));

    MOCK_METHOD(outcome::result<void>,
                execute_block_ref,
                (const primitives::BlockReflection &, TrieChangesTrackerOpt),
                (override));

    MOCK_METHOD(InitializeBlockResult,
                initialize_block,
                (const primitives::BlockHeader &, TrieChangesTrackerOpt),
                (override));
  };
}  // namespace kagome::runtime
