/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "dispute_coordinator/impl/candidate_vote_state.hpp"

namespace kagome::dispute {
  CandidateVoteState CandidateVoteState::create(CandidateVotes votes,
                                                CandidateEnvironment &env,
                                                Timestamp now) {
//
//		let own_vote = OwnVoteState::new(&votes, env);
//
//		let n_validators = env.validators().len();
//
//		let supermajority_threshold = polkadot_primitives::supermajority_threshold(n_validators);
//
//		// We have a dispute, if we have votes on both sides:
//		let is_disputed = !votes.invalid.is_empty() && !votes.valid.raw().is_empty();
//
//		let dispute_status = if is_disputed {
//			let mut status = DisputeStatus::active();
//			let byzantine_threshold = polkadot_primitives::byzantine_threshold(n_validators);
//			let is_confirmed = votes.voted_indices().len() > byzantine_threshold;
//			if is_confirmed {
//				status = status.confirm();
//			};
//			let concluded_for = votes.valid.raw().len() >= supermajority_threshold;
//			if concluded_for {
//				status = status.conclude_for(now);
//			};
//
//			let concluded_against = votes.invalid.len() >= supermajority_threshold;
//			if concluded_against {
//				status = status.conclude_against(now);
//			};
//			Some(status)
//		} else {
//			None
//		};
//
//		Self { votes, own_vote, dispute_status }


    return CandidateVoteState(); // TODO unstab
  }
}
