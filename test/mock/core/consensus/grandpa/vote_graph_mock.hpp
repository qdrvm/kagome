/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/grandpa/vote_graph.hpp"

#include <gmock/gmock.h>

namespace kagome::consensus::grandpa {

  class VoteGraphMock : public VoteGraph {
   public:
    MOCK_METHOD(const BlockInfo &, getBase, (), (const, override));

    MOCK_METHOD(void, adjustBase, (const std::vector<BlockHash> &), (override));

    MOCK_METHOD(outcome::result<void>,
                insert,
                (const BlockInfo &block, const VoteWeight &vote),
                (override));

    MOCK_METHOD(outcome::result<void>,
                insert,
                (const Prevote &prevote, const VoteWeight &vote),
                (override));

    MOCK_METHOD(outcome::result<void>,
                insert,
                (const Precommit &precommit, const VoteWeight &vote),
                (override));

    MOCK_METHOD(std::optional<BlockInfo>,
                findAncestor,
                (const BlockInfo &, const Condition &),
                (override));

    MOCK_METHOD(std::optional<BlockInfo>,
                findGhost,
                (const std::optional<BlockInfo> &, const Condition &),
                (override));
  };

}  // namespace kagome::consensus::grandpa
