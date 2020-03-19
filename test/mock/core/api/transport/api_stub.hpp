/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_MOCK_API_API_STUB_HPP
#define KAGOME_TEST_MOCK_API_API_STUB_HPP

#include <gmock/gmock.h>

#include <outcome/outcome.hpp>

namespace kagome::api {

  class ApiStub {
   public:
    outcome::result<int64_t> echo(int64_t payload) {
      return payload;
    }
  };

}  // namespace kagome::api

#endif  // KAGOME_TEST_MOCK_API_API_STUB_HPP
