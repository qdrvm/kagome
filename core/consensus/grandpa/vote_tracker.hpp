/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

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
    virtual PushResult push(const SignedMessage &vote, size_t weight) = 0;

    /**
     * Unpush a vote from a tracker (i.e. at wrong vote)
     * @param vote the voting message was pushed before
     * @param weight weight of this vote
     */
    virtual void unpush(const SignedMessage &vote, size_t weight) = 0;

    /**
     * @returns all accepted (non-duplicate) messages
     */
    virtual std::vector<VoteVariant> getMessages() const = 0;

    virtual std::optional<VoteVariant> getMessage(Id id) const = 0;

    /**
     * @returns total weight of all accepted (non-duplicate) messages
     */
    virtual size_t getTotalWeight() const = 0;
  };

}  // namespace kagome::consensus::grandpa
