/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
   * @tparam MessageType is the type of a message stored in the tracker. Note
   * that it is wrapped into a SignedMessage
   */
  template <typename MessageType>
  class VoteTracker {
   public:
    enum class PushResult { SUCCESS, DUPLICATED, EQUIVOCATED };
    using VotingMessage = SignedMessage<MessageType>;
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
