/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_MOCK_CORE_RUNTIME_CORE_MOCK_HPP
#define KAGOME_TEST_MOCK_CORE_RUNTIME_CORE_MOCK_HPP

#include "runtime/core.hpp"

#include <gmock/gmock.h>

namespace kagome::runtime {

  class CoreMock : public Core {
   public:
    MOCK_METHOD1(versionAt,
                 outcome::result<primitives::Version>(
                     primitives::BlockHash const &));
    MOCK_METHOD0(version,
                 outcome::result<primitives::Version>());
    MOCK_METHOD1(execute_block,
                 outcome::result<void>(const primitives::Block &));
    MOCK_METHOD1(initialise_block,
                 outcome::result<void>(const primitives::BlockHeader &));
    MOCK_METHOD1(authorities,
                 outcome::result<std::vector<primitives::AuthorityId>>(
                     const primitives::BlockId &));
  };
}  // namespace kagome::runtime

#endif  // KAGOME_TEST_MOCK_CORE_RUNTIME_CORE_MOCK_HPP
