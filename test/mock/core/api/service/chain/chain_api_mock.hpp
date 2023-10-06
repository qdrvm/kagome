/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gmock/gmock.h>

#include "api/service/chain/chain_api.hpp"

namespace kagome::api {

  class ChainApiMock : public ChainApi {
   public:
    ~ChainApiMock() override = default;

    MOCK_METHOD(outcome::result<BlockHash>,
                getBlockHash,
                (),
                (const, override));

    MOCK_METHOD(outcome::result<BlockHash>,
                getBlockHash,
                (BlockNumber),
                (const, override));

    MOCK_METHOD(outcome::result<BlockHash>,
                getBlockHash,
                (std::string_view),
                (const, override));

    MOCK_METHOD(outcome::result<std::vector<BlockHash>>,
                getBlockHash,
                (gsl::span<const ValueType>),
                (const, override));

    MOCK_METHOD(outcome::result<primitives::BlockHeader>,
                getHeader,
                (std::string_view),
                (override));

    MOCK_METHOD(outcome::result<primitives::BlockHeader>,
                getHeader,
                (),
                (override));

    MOCK_METHOD(outcome::result<primitives::BlockData>,
                getBlock,
                (std::string_view),
                (override));

    MOCK_METHOD(outcome::result<primitives::BlockData>,
                getBlock,
                (),
                (override));

    MOCK_METHOD(outcome::result<primitives::BlockHash>,
                getFinalizedHead,
                (),
                (const, override));

    MOCK_METHOD(outcome::result<uint32_t>,
                subscribeFinalizedHeads,
                (),
                (override));

    MOCK_METHOD(outcome::result<void>,
                unsubscribeFinalizedHeads,
                (uint32_t),
                (override));

    MOCK_METHOD(outcome::result<uint32_t>, subscribeNewHeads, (), (override));

    MOCK_METHOD(outcome::result<void>,
                unsubscribeNewHeads,
                (uint32_t),
                (override));
  };

}  // namespace kagome::api
