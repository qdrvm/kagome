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
    MOCK_METHOD1(setApiService, void(const std::shared_ptr<api::ApiService> &));

    MOCK_CONST_METHOD0(getBlockHash, outcome::result<BlockHash>());
    MOCK_CONST_METHOD1(getBlockHash, outcome::result<BlockHash>(BlockNumber));
    MOCK_CONST_METHOD1(getBlockHash,
                       outcome::result<BlockHash>(std::string_view));
    MOCK_CONST_METHOD1(
        getBlockHash,
        outcome::result<std::vector<BlockHash>>(gsl::span<const ValueType>));
    MOCK_METHOD1(getHeader,
                 outcome::result<primitives::BlockHeader>(std::string_view));
    MOCK_METHOD0(getHeader, outcome::result<primitives::BlockHeader>());
    MOCK_METHOD1(getBlock,
                 outcome::result<primitives::BlockData>(std::string_view));
    MOCK_METHOD0(getBlock, outcome::result<primitives::BlockData>());
    MOCK_CONST_METHOD0(getFinalizedHead, outcome::result<primitives::BlockHash>());

    MOCK_METHOD0(subscribeFinalizedHeads, outcome::result<uint32_t>());
    MOCK_METHOD1(unsubscribeFinalizedHeads, outcome::result<void>(uint32_t));
    MOCK_METHOD0(subscribeNewHeads, outcome::result<uint32_t>());
    MOCK_METHOD1(unsubscribeNewHeads, outcome::result<void>(uint32_t));
  };

}  // namespace kagome::api

#endif  // KAGOME_CHAIN_API_MOCK_HPP
