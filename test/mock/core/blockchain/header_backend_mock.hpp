/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_MOCK_BLOCKCHAIN_HEADER_BACKEND_MOCK_HPP
#define KAGOME_TEST_MOCK_BLOCKCHAIN_HEADER_BACKEND_MOCK_HPP

#include "blockchain/header_backend.hpp"

#include <gmock/gmock.h>

namespace kagome::blockchain {

  class HeaderBackendMock : public HeaderBackend {
   public:
    MOCK_CONST_METHOD1(
        hashFromBlockId,
        outcome::result<common::Hash256>(const primitives::BlockId &));
    MOCK_CONST_METHOD1(
        numberFromBlockId,
        outcome::result<primitives::BlockNumber>(const primitives::BlockId &));
  };
}  // namespace kagome::blockchain

#endif  // KAGOME_TEST_MOCK_BLOCKCHAIN_HEADER_BACKEND_MOCK_HPP
