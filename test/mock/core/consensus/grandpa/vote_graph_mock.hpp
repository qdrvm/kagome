/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_MOCK_CORE_CONSENSUS_GRANDPA_VOTE_GRAPH_MOCK_HPP
#define KAGOME_TEST_MOCK_CORE_CONSENSUS_GRANDPA_VOTE_GRAPH_MOCK_HPP

#include "consensus/grandpa/vote_graph.hpp"

#include <gmock/gmock.h>

namespace kagome::consensus::grandpa {

  class VoteGraphMock : public VoteGraph {
   public:
    MOCK_CONST_METHOD0(getBase, const BlockInfo &());
    MOCK_METHOD1(adjustBase, void(const std::vector<BlockHash> &));
    MOCK_METHOD2(insert,
                 outcome::result<void>(const BlockInfo &block,
                                       const VoteWeight &vote));

    MOCK_METHOD2(insert,
                 outcome::result<void>(const Prevote &prevote,
                                       const VoteWeight &vote));

    MOCK_METHOD2(insert,
                 outcome::result<void>(const Precommit &precommit,
                                       const VoteWeight &vote));

    MOCK_METHOD2(findAncestor,
                 std::optional<BlockInfo>(const BlockInfo &,
                                          const Condition &));
    MOCK_METHOD2(findGhost,
                 std::optional<BlockInfo>(const std::optional<BlockInfo> &,
                                          const Condition &));
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_TEST_MOCK_CORE_CONSENSUS_GRANDPA_VOTE_GRAPH_MOCK_HPP
