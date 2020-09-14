/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CONSENSUS_GRANDPA_VOTINGROUNDIMPL
#define KAGOME_CORE_CONSENSUS_GRANDPA_VOTINGROUNDIMPL

#include "consensus/grandpa/voting_round.hpp"

#include <boost/asio/basic_waitable_timer.hpp>
#include <boost/signals2.hpp>

#include "common/logger.hpp"
#include "consensus/grandpa/environment.hpp"
#include "consensus/grandpa/grandpa_config.hpp"
#include "consensus/grandpa/movable_round_state.hpp"
#include "consensus/grandpa/structs.hpp"
#include "consensus/grandpa/vote_crypto_provider.hpp"
#include "consensus/grandpa/vote_graph.hpp"
#include "consensus/grandpa/vote_tracker.hpp"

namespace kagome::consensus::grandpa {

  class Grandpa;

  class VotingRoundImpl : public VotingRound {
    using OnCompleted = boost::signals2::signal<void(const CompletedRound &)>;
    using OnCompletedSlotType = OnCompleted::slot_type;

   private:
    VotingRoundImpl(const std::shared_ptr<Grandpa> &grandpa,
                    const GrandpaConfig &config,
                    std::shared_ptr<Environment> env,
                    std::shared_ptr<VoteCryptoProvider> vote_crypto_provider,
                    std::shared_ptr<VoteTracker> prevotes,
                    std::shared_ptr<VoteTracker> precommits,
                    std::shared_ptr<VoteGraph> graph,
                    std::shared_ptr<Clock> clock,
                    std::shared_ptr<boost::asio::io_context> io_context);

   public:
    ~VotingRoundImpl() override = default;

    VotingRoundImpl(
        const std::shared_ptr<Grandpa> &grandpa,
        const GrandpaConfig &config,
        const std::shared_ptr<Environment> &env,
        const std::shared_ptr<VoteCryptoProvider> &vote_crypto_provider,
        const std::shared_ptr<VoteTracker> &prevotes,
        const std::shared_ptr<VoteTracker> &precommits,
        const std::shared_ptr<VoteGraph> &graph,
        const std::shared_ptr<Clock> &clock,
        const std::shared_ptr<boost::asio::io_context> &io_context,
        const MovableRoundState& round_state);

    VotingRoundImpl(
        const std::shared_ptr<Grandpa> &grandpa,
        const GrandpaConfig &config,
        const std::shared_ptr<Environment> &env,
        const std::shared_ptr<VoteCryptoProvider> &vote_crypto_provider,
        const std::shared_ptr<VoteTracker> &prevotes,
        const std::shared_ptr<VoteTracker> &precommits,
        const std::shared_ptr<VoteGraph> &graph,
        const std::shared_ptr<Clock> &clock,
        const std::shared_ptr<boost::asio::io_context> &io_context,
        const std::shared_ptr<VotingRound> &previous_round);

    enum class Stage {
      // Initial stage, round is just created
      INIT,

      // Beginner stage, round is just start to play
      START,

      // Stages for prevote mechanism
      START_PREVOTE,
      PREVOTE_RUNS,
      END_PREVOTE,

      // Stages for precommit mechanism
      START_PRECOMMIT,
      PRECOMMIT_RUNS,
      END_PRECOMMIT,

      // Stages for waiting finalisation
      START_WAITING,
      WAITING_RUNS,
      END_WAITING,

      // Final state. Round was finalized
      COMPLETED
    };

    // Start/stop round

    void play() override;
    void end() override;

    // Workflow of round

    void startPrevoteStage();
    void endPrevoteStage();
    void startPrecommitStage();
    void endPrecommitStage();
    void startWaitingStage();
    void endWaitingStage();

    // Actions of round

    void doProposal() override;
    void doPrevote() override;
    void doPrecommit() override;
    void doFinalize() override;

    // Handlers of incoming messages

    /**
     * Triggered when we receive finalization message
     */
    void onFinalize(const Fin &finalize) override;

    /**
     * Invoked when we received a primary propose for the current round
     * Basically method just checks if received propose was produced by the
     * primary and if so, it is stored in primary_vote_ field
     */
    void onProposal(const SignedMessage &proposal) override;

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
     */
    void attemptToFinalizeRound() override;

    // Catch-up actions

    void doCatchUpRequest(const libp2p::peer::PeerId &peer_id) override;
    void doCatchUpResponse(const libp2p::peer::PeerId &peer_id) override;

    // Getters

    RoundNumber roundNumber() const override;
    MembershipCounter voterSetId() const override;
//    std::shared_ptr<const RoundState> state() const override;

    /**
     * Round is completable when we have block (stored in
     * current_state_.finalized) for which we have supermajority on both
     * prevotes and precommits
     */
    bool completable() const override;

    bool finalizable() const override;

    BlockInfo lastFinalizedBlock() const override {
      return last_finalized_block_;
    }
    BlockInfo bestPrevoteCandidate() override;
    BlockInfo bestFinalCandidate() override;
    boost::optional<BlockInfo> finalizedBlock() const override {
      return finalized_;
    };

		MovableRoundState state() const override;

   private:
    /// Check if peer \param id is primary
    bool isPrimary(const Id &id) const;

    /// Check if current peer is primary
    bool isPrimary() const;

    /// Calculate threshold from the total weights of voters
    size_t getThreshold(const std::shared_ptr<VoterSet> &voters);

    /// Triggered when we receive \param signed_prevote for the current peer
    outcome::result<void> onSignedPrevote(const SignedMessage &signed_prevote);

    /// Triggered when we receive \param signed_precommit for the current peer
    outcome::result<void> onSignedPrecommit(
        const SignedMessage &signed_precommit);

    /**
     * Invoked during each onSingedPrevote.
     * Updates current round's prevote ghost. New prevote-ghost is the highest
     * block with supermajority of prevotes
     */
    void updatePrevoteGhost();

    /// Update current state of the round.
    /// In particular we update:
    /// 1. If round is completable
    /// 2. If round has something ready to finalize
    /// 3. New best rounds estimate
    void update();

    // notify about new finalized round. False if new state does not differ from
    // old one
    outcome::result<void> notify();

    /// prepare justification of \param estimate over the provided \param votes
    boost::optional<GrandpaJustification> getJustification(
        const BlockInfo &estimate, const std::vector<VoteVariant> &votes) const;

    /// @see spec: Grandpa-Ghost
    BlockInfo grandpaGhost();

    /**
     * We will vote for the best chain containing primary_vote_ iff
     * the last round's prevote-GHOST included that block and
     * that block is a strict descendent of the last round-estimate that we are
     * aware of.
     * Otherwise we will vote for the best chain containing last round estimate
     */
    outcome::result<Prevote> calculatePrevote() const;

    /// Check if received \param vote has valid \param justification prevote
    bool validatePrevoteJustification(
        const BlockInfo &vote, const GrandpaJustification &justification) const;

    /// Check if received \param vote has valid \param justification precommit
    bool validatePrecommitJustification(
        const BlockInfo &vote, const GrandpaJustification &justification) const;

    void sendProposal(const PrimaryPropose &primary_proposal);
    void sendPrevote(const Prevote &prevote);
    void sendPrecommit(const Precommit &precommit);
    void sendFinalize(const BlockInfo &block,
                      const GrandpaJustification &justification);

    void pending();

		std::shared_ptr<VoterSet> voter_set_;
		const RoundNumber round_number_;
		std::weak_ptr<VotingRound> previous_round_;

		const Duration duration_;  // length of round (T in spec)
		bool isPrimary_ = false;
		size_t threshold_;  // supermajority threshold
		const Id id_;  // id of current peer
		TimePoint start_time_;

		std::weak_ptr<Grandpa> grandpa_;
		std::shared_ptr<Environment> env_;
		std::shared_ptr<VoteCryptoProvider> vote_crypto_provider_;
		std::shared_ptr<VoteGraph> graph_;
		std::shared_ptr<Clock> clock_;
		std::shared_ptr<boost::asio::io_context> io_context_;

		std::function<void()> on_complete_handler_;

		Stage stage_ = Stage::INIT;

		std::shared_ptr<VoteTracker> prevotes_;
		std::shared_ptr<VoteTracker> precommits_;

		// equivocators arrays. Index in vector corresponds to the index of voter in
		// voterset, value corresponds to the weight of the voter
		std::vector<bool> prevote_equivocators_;
		std::vector<bool> precommit_equivocators_;

		boost::optional<PrimaryPropose> primary_vote_;
		boost::optional<Prevote> prevote_;
		boost::optional<Precommit> precommit_;

		BlockInfo last_finalized_block_;
		BlockInfo best_prevote_candidate_;
		BlockInfo best_final_candidate_;
		boost::optional<BlockInfo> finalized_;

    Timer timer_;
    Timer pending_timer_;

    common::Logger logger_ = common::createLogger("Grandpa");

    bool completable_ = false;
  };
}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CORE_CONSENSUS_GRANDPA_VOTINGROUNDIMPL
