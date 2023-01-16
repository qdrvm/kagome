/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_MOCK_CORE_RUNTIME_RAW_EXECUTOR_MOCK_HPP
#define KAGOME_TEST_MOCK_CORE_RUNTIME_RAW_EXECUTOR_MOCK_HPP

#include "runtime/raw_executor.hpp"

#include <gmock/gmock.h>

namespace kagome::runtime {

  class RawExecutorMock : public RawExecutor {
   public:
    MOCK_METHOD(outcome::result<common::Buffer>,
                callAtRaw,
                (const primitives::BlockHash &,
                 std::string_view,
                 const common::Buffer &,
                 OnDbRead),
                (override));
  };

}  // namespace kagome::runtime

#endif  // KAGOME_TEST_MOCK_CORE_RUNTIME_RAW_EXECUTOR_MOCK_HPP
