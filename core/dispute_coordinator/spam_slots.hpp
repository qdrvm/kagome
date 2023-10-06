/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_DISPUTE_SPAMSLOTS
#define KAGOME_DISPUTE_SPAMSLOTS

#include "dispute_coordinator/types.hpp"
#include "network/types/collator_messages.hpp"
#include "parachain/types.hpp"

namespace kagome::dispute {

  using network::CandidateHash;
  using network::SessionIndex;
  using network::ValidatorIndex;

  class SpamSlots {
   public:
    virtual ~SpamSlots() = default;

    /// Increase a "voting invalid" validator's spam slot.
    ///
    /// This function should get called for any validator's invalidity vote for
    /// any not yet confirmed dispute.
    ///
    /// Returns: `true` if validator still had vacant spam slots, `false`
    /// otherwise.
    virtual bool add_unconfirmed(SessionIndex session,
                                 CandidateHash candidate,
                                 ValidatorIndex validator) = 0;

    /// Clear out spam slots for a given candidate in a session.
    ///
    /// This effectively reduces the spam slot count for all validators
    /// participating in a dispute for that candidate. You should call this
    /// function once a dispute became obsolete or got confirmed and thus votes
    /// for it should no longer be treated as potential spam.
    virtual void clear(SessionIndex validator, CandidateHash candidate) = 0;

    /// Prune all spam slots for sessions older than the given index.
    virtual void prune_old(SessionIndex oldest_index) = 0;
  };

}  // namespace kagome::dispute

#endif  // KAGOME_DISPUTE_SPAMSLOTS
