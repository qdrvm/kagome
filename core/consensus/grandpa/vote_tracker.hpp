/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CONSENSUS_GRANDPA_VOTE_TRACKER_HPP
#define KAGOME_CORE_CONSENSUS_GRANDPA_VOTE_TRACKER_HPP

#include "consensus/grandpa/structs.hpp"

namespace kagome::consensus::grandpa {

  /**
   * Stores voting messages (such as prevotes and precommits) during a round,
   * reports if a messages that is being put into it is a duplicate or an
   * equivote (which is the case when a node votes for two different blocks
   * during a round)
   */
  class VoteTracker {
   public:
    enum class PushResult { SUCCESS, DUPLICATED, EQUIVOCATED };
    using VotingMessage = SignedMessage;
    using EquivocatoryVotingMessage = std::pair<VotingMessage, VotingMessage>;
    using VoteVariant =
        boost::variant<VotingMessage, EquivocatoryVotingMessage>;

    virtual ~VoteTracker() = default;
    /**
     * Attempts to push a vote to a tracker
     * @param vote the voting message being pushed
     * @param weight weight of this vote
     * @returns SUCCESS if it is the first pushed vote of the voter, EQUIVOCATED
     * if the voting peer already voted for a different block, DUPLICATE if the
     * peer already voted for another block more than once or voted for the same
     * block
     */
    virtual PushResult push(const VotingMessage &vote, size_t weight) = 0;

    /**
     * @returns all accepted (non-duplicate) messages
     */
    virtual std::vector<VoteVariant> getMessages() const = 0;

    /**
     * @returns total weight of all accepted (non-duplicate) messages
     */
    virtual size_t getTotalWeight() const = 0;
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CORE_CONSENSUS_GRANDPA_VOTE_TRACKER_HPP
