/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_DISPUTE_SPAMSLOTSIMPL
#define KAGOME_DISPUTE_SPAMSLOTSIMPL

#include "dispute_coordinator/spam_slots.hpp"

#include <set>
#include <tuple>
#include <unordered_map>

#include "dispute_coordinator/types.hpp"
#include "network/types/collator_messages.hpp"
#include "parachain/types.hpp"
#include "utils/tuple_hash.hpp"

namespace kagome::dispute {
  using network::CandidateHash;
  using network::SessionIndex;
  using network::ValidatorIndex;

  class SpamSlotsImpl final : public SpamSlots {
   public:
    using SpamCount = uint32_t;
    static const SpamCount kMaxSpamVotes = 50;

    using Slots =
        std::unordered_map<std::tuple<SessionIndex, ValidatorIndex>, SpamCount>;

    /// Unconfirmed disputes to be passed at initialization.
    using UnconfirmedDisputes =
        std::unordered_map<std::tuple<SessionIndex, CandidateHash>,
                           std::set<ValidatorIndex>>;

    SpamSlotsImpl(Slots slots, UnconfirmedDisputes unconfirmed_disputes)
        : slots_(std::move(slots)),
          unconfirmed_(std::move(unconfirmed_disputes)){};

    bool add_unconfirmed(SessionIndex session,
                         CandidateHash candidate,
                         ValidatorIndex validator) override;

    void clear(SessionIndex validator, CandidateHash candidate) override;

    void prune_old(SessionIndex oldest_index) override;

   private:
    /// Counts per validator and session.
    ///
    /// Must not exceed `MAX_SPAM_VOTES`.
    Slots slots_;

    /// All unconfirmed candidates we are aware of right now.
    UnconfirmedDisputes unconfirmed_;
  };

}  // namespace kagome::dispute

#endif  // KAGOME_DISPUTE_SPAMSLOTSIMPL
