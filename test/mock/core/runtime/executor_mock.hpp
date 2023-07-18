/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_MOCK_CORE_RUNTIME_RAW_EXECUTOR_MOCK_HPP
#define KAGOME_TEST_MOCK_CORE_RUNTIME_RAW_EXECUTOR_MOCK_HPP

#include "runtime/executor.hpp"

#include <gmock/gmock.h>

namespace kagome::runtime {

  class ExecutorMock : public Executor {
   public:
    MOCK_METHOD(
        outcome::result<std::unique_ptr<RuntimeContext>>,
        getPersistentContextAt,
        (const primitives::BlockHash &block_hash,
         std::optional<std::shared_ptr<storage::changes_trie::ChangesTracker>>
             changes_tracker),
        (override));

    MOCK_METHOD(outcome::result<std::unique_ptr<RuntimeContext>>,
                getEphemeralContextAt,
                (const primitives::BlockHash &block_hash),
                (override));

    MOCK_METHOD(outcome::result<std::unique_ptr<RuntimeContext>>,
                getEphemeralContextAt,
                (const primitives::BlockHash &block_hash,
                 const storage::trie::RootHash &state_hash),
                (override));

    MOCK_METHOD(outcome::result<std::unique_ptr<RuntimeContext>>,
                getEphemeralContextAtGenesis,
                (),
                (override));

    MOCK_METHOD(outcome::result<Buffer>,
                callWithCtx,
                (RuntimeContext & ctx,
                 std::string_view name,
                 const Buffer &encoded_args),
                (override));
  };

}  // namespace kagome::runtime

#endif  // KAGOME_TEST_MOCK_CORE_RUNTIME_RAW_EXECUTOR_MOCK_HPP
