/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CONSENSUS_GRANDPA_IMPL_VOTING_ROUND_IMPL_HPP
#define KAGOME_CORE_CONSENSUS_GRANDPA_IMPL_VOTING_ROUND_IMPL_HPP

#include "consensus/grandpa/voting_round.hpp"

#include <boost/asio/basic_waitable_timer.hpp>
#include "blockchain/block_tree.hpp"
#include "common/logger.hpp"
#include "consensus/grandpa/chain.hpp"
#include "consensus/grandpa/gossiper.hpp"
#include "consensus/grandpa/vote_graph.hpp"
#include "consensus/grandpa/vote_tracker.hpp"
#include "crypto/ed25519_provider.hpp"

namespace kagome::consensus::grandpa {

  class VotingRoundImpl : public VotingRound {
   public:
    ~VotingRoundImpl() override = default;

    VotingRoundImpl(std::shared_ptr<VoterSet> voters,
                    RoundNumber round_number,
                    Duration duration,
                    TimePoint start_time,
                    MembershipCounter counter,
                    RoundState last_round_state,
                    crypto::ED25519Keypair keypair,
                    std::shared_ptr<VoteTracker> tracker,
                    std::shared_ptr<VoteGraph> graph,
                    std::shared_ptr<Gossiper> gossiper,
                    std::shared_ptr<crypto::ED25519Provider> ed_provider,
                    std::shared_ptr<Clock> clock,
                    std::shared_ptr<blockchain::BlockTree> block_tree,
                    Timer timer,
                    common::Logger logger = common::createLogger("Grandpa"));

    void onFin(const Fin &f);

    void onPrimaryPropose(const SignedPrimaryPropose &primary_propose) override;

    void onPrevote(const SignedPrevote &prevore) override;

    void onPrecommit(const SignedPrecommit &precommit) override;

    void onVoteMessage(const VoteMessage &vote_message);

    void tryFinalize() override;

    void primaryPropose(const RoundState &last_round_state) override;

    void prevote(const RoundState &last_round_state) override;

    void precommit(const RoundState &last_round_state) override;

    inline const RoundState &getCurrentState() const {
      return cur_round_state_;
    }

    bool completable() const;

   private:
    bool isPrimary(const Id &id) const;
    bool isPrimary() const;

    size_t getThreshold(const std::shared_ptr<VoterSet> &voters);
    /**
     * @tparam VoteType – either SignedPrevote or SignedPrecommit
     * @param vote – vote is pushed to tracker if it is not equivocated vote and
     * it was not pushed into the tracker before
     */
    template <typename VoteType>
    void onVote(const VoteType &vote);

    void updatePrevoteGhost();

    void update();

    outcome::result<SignedPrevote> constructPrevote(
        const RoundState &last_round_state) const;

    outcome::result<SignedPrecommit> constructPrecommit(
        const RoundState &last_round_state) const;

    void gossipPrimaryPropose(const SignedPrimaryPropose &primary_propose);

    void gossipPrevote(const SignedPrevote &prevote);

    void gossipPrecommit(const SignedPrecommit &precommit);

    template <typename VoteType>
    crypto::ED25519Signature voteSignature(uint8_t stage,
                                           const VoteType &vote_type) const;

    SignedPrimaryPropose signPrimaryPropose(
        const PrimaryPropose &primary_propose) const;

    SignedPrevote signPrevote(const Prevote &prevote) const;

    SignedPrecommit signPrecommit(const Precommit &precommit) const;

   private:
    std::shared_ptr<VoterSet> voter_set_;
    const RoundNumber round_number_;
    const Duration duration_;  // length of round (T in spec)
    const TimePoint start_time_;
    const MembershipCounter counter_;
    RoundState last_round_state_;
    RoundState cur_round_state_;
    const crypto::ED25519Keypair keypair_;
    const Id id_;  // id of current peer
    State state_;
    size_t threshold_;

    std::shared_ptr<VoteTracker> tracker_;
    std::shared_ptr<VoteGraph> graph_;

    std::shared_ptr<Gossiper> gossiper_;
    std::shared_ptr<crypto::ED25519Provider> ed_provider_;
    std::shared_ptr<Clock> clock_;
    std::shared_ptr<blockchain::BlockTree> block_tree_;
    std::shared_ptr<Chain> chain_;

    Timer timer_;

    common::Logger logger_;

    boost::optional<PrimaryPropose> primaty_vote_;
    bool completable_{false};
  };
}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CORE_CONSENSUS_GRANDPA_IMPL_VOTING_ROUND_IMPL_HPP
