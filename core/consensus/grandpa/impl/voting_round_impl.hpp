/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CONSENSUS_GRANDPA_IMPL_VOTING_ROUND_IMPL_HPP
#define KAGOME_CORE_CONSENSUS_GRANDPA_IMPL_VOTING_ROUND_IMPL_HPP

#include "consensus/grandpa/voting_round.hpp"

#include <boost/asio/basic_waitable_timer.hpp>
#include <boost/signals2.hpp>

#include "common/logger.hpp"
#include "consensus/grandpa/environment.hpp"
#include "consensus/grandpa/grandpa_config.hpp"
#include "consensus/grandpa/vote_crypto_provider.hpp"
#include "consensus/grandpa/vote_graph.hpp"
#include "consensus/grandpa/vote_tracker.hpp"

namespace kagome::consensus::grandpa {

  class VotingRoundImpl : public VotingRound {
    using OnCompleted = boost::signals2::signal<void(const CompletedRound &)>;
    using OnCompletedSlotType = OnCompleted::slot_type;

   public:
    ~VotingRoundImpl() override = default;

    VotingRoundImpl(const GrandpaConfig &config,
                    std::shared_ptr<Environment> env,
                    std::shared_ptr<VoteCryptoProvider> vote_crypto_provider,
                    std::shared_ptr<VoteTracker> prevotes,
                    std::shared_ptr<VoteTracker> precommits,
                    std::shared_ptr<VoteGraph> graph,
                    std::shared_ptr<Clock> clock,
                    std::shared_ptr<boost::asio::io_context> io_context);

    /**
     * Triggered when we receive finalization message
     */
    void onFinalize(const Fin &f) override;

    /**
     * Invoked when we received a primary propose for the current round
     * Basically method just checks if received propose was produced by the
     * primary and if so, it is stored in primary_vote_ field
     */
    void onPrimaryPropose(const SignedMessage &primary_propose) override;

    /**
     * Triggered when we receive prevote for current round
     * \param prevote is stored in prevote tracker and vote graph
     * Then we try to update prevote ghost (\see updatePrevoteGhost) and round
     * state (\see update)
     */
    void onPrevote(const SignedMessage &prevote) override;

    /**
     * Triggered when we receive precommit for the current round
     * \param precommit is stored in precommit tracker and vote graph
     * Then we try to update round state and finalize
     */
    void onPrecommit(const SignedMessage &precommit) override;

    /**
     * Checks if current round is completable and finalized block differs from
     * the last round's finalized block. If so fin message is broadcasted to the
     * network
     * @return true if fin message was sent, false otherwise
     */
    bool tryFinalize() override;

    RoundNumber roundNumber() const override;

    /**
     * During the primary propose we :
     * 1. Check if we are the primary for the current round. If not execution of
     * the method is finished
     * 2. We can send primary propose only if the estimate from \param
     * last_round_state is greater than finalized from \param last_round_state.
     * If we cannot send propose, method is finished
     * 3. Primary propose is the last rounds estimate.
     * 4. After all steps above are done we broadcast propose
     * 5. We store what we have broadcasted in primary_vote_ field
     */
    void primaryPropose(const RoundState &last_round_state) override;

    /**
     * 1. Waits until start_time_ + duration * 2 or round is completable
     * 2. Constructs prevote (\see constructPrevote) and broadcasts it
     */
    void prevote(const RoundState &last_round_state) override;

    /**
     * 1. Waits until start_time_ + duration * 4 or round is completable
     * 2. Constructs precommit (\see constructPrecommit) and broadcasts it
     */
    void precommit(const RoundState &last_round_state) override;

    inline const RoundState &getCurrentState() const {
      return cur_round_state_;
    }

    /**
     * Round is completable when we have block (stored in
     * current_state_.finalized) for which we have supermajority on both
     * prevotes and precommits
     */
    bool completable() const;

   private:
    /// Check if peer \param id is primary
    bool isPrimary(const Id &id) const;

    /// Check if current peer is primary
    bool isPrimary() const;

    /// Calculate threshold from the total weights of voters
    size_t getThreshold(const std::shared_ptr<VoterSet> &voters);

    /// Triggered when we receive \param signed_prevote for the current peer
    void onSignedPrevote(const SignedMessage &signed_prevote);

    /// Triggered when we receive \param signed_precommit for the current peer
    bool onSignedPrecommit(const SignedMessage &signed_precommit);

    /**
     * Invoked during each onSingedPrevote.
     * Updates current round's prevote ghost. New prevote-ghost is the highest
     * block with supermajority of prevotes
     */
    void updatePrevoteGhost();

    /// Update current state of the round. In particular we update:
    /// 1. If round is completable
    /// 2. If round has something ready to finalize
    /// 3. New best rounds estimate
    void update();

    // notify about new finalized round. False if new state does not differ from
    // old one
    outcome::result<void> notify(const RoundState &last_round_state);

    /// prepare justification for the provided \param estimate
    boost::optional<GrandpaJustification> finalizingPrecommits(
        const BlockInfo &estimate) const;

    /**
     * We will vote for the best chain containing primary_vote_ iff
     * the last round's prevote-GHOST included that block and
     * that block is a strict descendent of the last round-estimate that we are
     * aware of.
     * Otherwise we will vote for the best chain containing last round estimate
     */
    outcome::result<SignedMessage> constructPrevote(
        const RoundState &last_round_state) const;

    /**
     * Constructs precommit as following:
     * 1. If we have prevote ghost, then vote for it
     * 2. Otherwise vote for the last finalized block (base of the graph)
     */
    outcome::result<SignedMessage> constructPrecommit(
        const RoundState &last_round_state) const;

    /// Check if received \param vote has valid \param justification. If so
    /// \return true, false otherwise
    bool validate(const BlockInfo &vote,
                  const GrandpaJustification &justification) const;

   private:
    std::shared_ptr<VoterSet> voter_set_;
    const RoundNumber round_number_;
    const Duration duration_;  // length of round (T in spec)
    TimePoint start_time_;
    RoundState cur_round_state_;

    boost::optional<RoundState> last_round_state_;
    const Id id_;  // id of current peer

    std::shared_ptr<Environment> env_;

    std::shared_ptr<VoteCryptoProvider> vote_crypto_provider_;
    State state_;
    size_t threshold_;  // supermajority threshold

    std::shared_ptr<VoteTracker> prevotes_;
    std::shared_ptr<VoteTracker> precommits_;
    std::shared_ptr<VoteGraph> graph_;

    std::shared_ptr<Clock> clock_;

    std::shared_ptr<boost::asio::io_context> io_context_;
    Timer prevote_timer_;
    Timer precommit_timer_;

    common::Logger logger_;
    // equivocators arrays. Index in vector corresponds to the index of voter in
    // voterset, value corresponds to the weight of the voter
    std::vector<bool> prevote_equivocators_;
    std::vector<bool> precommit_equivocators_;

    boost::optional<PrimaryPropose> primary_vote_;
    bool completable_{false};
  };
}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CORE_CONSENSUS_GRANDPA_IMPL_VOTING_ROUND_IMPL_HPP
