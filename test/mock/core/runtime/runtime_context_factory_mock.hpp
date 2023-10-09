/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "runtime/runtime_context.hpp"

#include <gmock/gmock.h>

namespace kagome::runtime {

  class RuntimeContextFactoryMock : public RuntimeContextFactory {
   public:
    MOCK_METHOD(outcome::result<RuntimeContext>,
                fromBatch,
                (std::shared_ptr<ModuleInstance> module_instance,
                 std::shared_ptr<storage::trie::TrieBatch> batch,
                 ContextParams params),
                (override));
    MOCK_METHOD(
        outcome::result<RuntimeContext>,
        persistent,
        (std::shared_ptr<ModuleInstance> module_instance,
         const storage::trie::RootHash &state,
         std::optional<std::shared_ptr<storage::changes_trie::ChangesTracker>>
             changes_tracker_opt,
         ContextParams params),
        (override));
    MOCK_METHOD(
        outcome::result<RuntimeContext>,
        persistentAt,
        (const primitives::BlockHash &block_hash,
         std::optional<std::shared_ptr<storage::changes_trie::ChangesTracker>>
             changes_tracker_opt,
         ContextParams params),
        (override));
    MOCK_METHOD(outcome::result<RuntimeContext>,
                ephemeral,
                (std::shared_ptr<ModuleInstance> module_instance,
                 const storage::trie::RootHash &state,
                 ContextParams params),
                (override));
    MOCK_METHOD(outcome::result<RuntimeContext>,
                ephemeralAt,
                (const primitives::BlockHash &block_hash, ContextParams params),
                (override));
    MOCK_METHOD(outcome::result<RuntimeContext>,
                ephemeralAt,
                (const primitives::BlockHash &block_hash,
                 const storage::trie::RootHash &state,
                 ContextParams params),
                (override));
    MOCK_METHOD(outcome::result<RuntimeContext>,
                ephemeralAtGenesis,
                (ContextParams params),
                (override));
  };

}  // namespace kagome::runtime
