/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CHAIN_API_MOCK_HPP
#define KAGOME_CHAIN_API_MOCK_HPP

#include <gmock/gmock.h>

#include "api/service/chain/chain_api.hpp"

namespace kagome::api {

  class ChainApiMock : public ChainApi {
   public:
    ~ChainApiMock() override = default;

    MOCK_CONST_METHOD0(getBlockHash, outcome::result<BlockHash>());
    MOCK_CONST_METHOD1(getBlockHash, outcome::result<BlockHash>(uint32_t));
    MOCK_CONST_METHOD1(getBlockHash,
                       outcome::result<BlockHash>(std::string_view));
    MOCK_CONST_METHOD1(
        getBlockHash,
        outcome::result<std::vector<BlockHash>>(gsl::span<const ValueType>));
  };

}  // namespace kagome::api

#endif  // KAGOME_CHAIN_API_MOCK_HPP
