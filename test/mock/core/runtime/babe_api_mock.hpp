/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_MOCK_CORE_RUNTIME_BABE_API_MOCK_HPP
#define KAGOME_TEST_MOCK_CORE_RUNTIME_BABE_API_MOCK_HPP

#include "runtime/babe_api.hpp"

#include <gmock/gmock.h>

namespace kagome::runtime {

  class BabeApiMock : public BabeApi {
   public:
    MOCK_METHOD0(configuration,
                 outcome::result<primitives::BabeConfiguration>());
  };

}  // namespace kagome::runtime

#endif  // KAGOME_TEST_MOCK_CORE_RUNTIME_BABE_API_MOCK_HPP
