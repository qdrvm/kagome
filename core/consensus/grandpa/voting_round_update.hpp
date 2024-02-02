/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/grandpa/voting_round.hpp"

namespace kagome::consensus::grandpa {
  /**
   * Convenience wrapper for batch vote import.
   * Import multiple `.vote(vote)`, then `.update()`.
   */
  struct VotingRoundUpdate {
    VotingRound &round;
    std::optional<GrandpaContext> ctx;
    bool propagate = false;
    bool update_prevote = false;
    bool update_precommit = false;

    void vote(const SignedMessage &msg) {
      auto propagate2 = VotingRound::Propagation{propagate};
      if (msg.is<PrimaryPropose>()) {
        round.onProposal(ctx, msg, propagate2);
      } else if (msg.is<Prevote>()) {
        if (round.onPrevote(ctx, msg, propagate2)) {
          update_prevote = true;
        }
      } else if (msg.is<Precommit>()) {
        if (round.onPrecommit(ctx, msg, propagate2)) {
          update_precommit = true;
        }
      }
    }

    void vote(const VoteVariant &msg) {
      if (auto e = boost::get<EquivocatorySignedMessage>(&msg)) {
        vote(e->first);
        vote(e->second);
      } else {
        vote(boost::get<SignedMessage>(msg));
      }
    }

    bool changed() const {
      return update_prevote or update_precommit;
    }

    void update() {
      round.update(VotingRound::IsPreviousRoundChanged{false},
                   VotingRound::IsPrevotesChanged{update_prevote},
                   VotingRound::IsPrecommitsChanged{update_precommit});
    }
  };
}  // namespace kagome::consensus::grandpa
