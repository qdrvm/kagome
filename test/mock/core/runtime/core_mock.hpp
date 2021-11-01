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
                (primitives::BlockHash const &block),
                (override));

    MOCK_METHOD(outcome::result<void>,
                execute_block,
                (const primitives::Block &),
                (override));

    MOCK_METHOD(outcome::result<storage::trie::RootHash>,
                initialize_block,
                (const primitives::BlockHeader &),
                (override));

    MOCK_METHOD(outcome::result<std::vector<primitives::AuthorityId>>,
                authorities,
                (const primitives::BlockHash &),
                (override));
  };
}  // namespace kagome::runtime

#endif  // KAGOME_TEST_MOCK_CORE_RUNTIME_CORE_MOCK_HPP
