/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_MOCK_CORE_RUNTIME_CORE_MOCK_HPP
#define KAGOME_TEST_MOCK_CORE_RUNTIME_CORE_MOCK_HPP

#include "runtime/runtime_api/core.hpp"

#include <gmock/gmock.h>

namespace kagome::runtime {

  class CoreMock : public Core {
   public:
    MOCK_METHOD(outcome::result<primitives::Version>, version, (), (override));

    MOCK_METHOD(outcome::result<primitives::Version>,
                version,
                (std::shared_ptr<ModuleInstance>),
                (override));

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

    MOCK_METHOD(outcome::result<std::unique_ptr<RuntimeContext>>,
                initialize_block,
                (const primitives::BlockHeader &, TrieChangesTrackerOpt),
                (override));
  };
}  // namespace kagome::runtime

#endif  // KAGOME_TEST_MOCK_CORE_RUNTIME_CORE_MOCK_HPP
