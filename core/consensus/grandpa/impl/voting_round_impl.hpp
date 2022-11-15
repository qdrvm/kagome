/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_GRANDPA_VOTINGROUNDIMPL
#define KAGOME_CONSENSUS_GRANDPA_VOTINGROUNDIMPL

#include "consensus/grandpa/voting_round.hpp"

#include <libp2p/basic/scheduler.hpp>

#include "log/logger.hpp"

namespace kagome::consensus::grandpa {
  class AuthorityManager;
  class Environment;
  class Grandpa;
  struct GrandpaConfig;
  struct MovableRoundState;
  class VoteCryptoProvider;
  class VoteGraph;
  class VoteTracker;
  class VoterSet;
}  // namespace kagome::consensus::grandpa

namespace kagome::consensus::grandpa {

  class VotingRoundImpl : public VotingRound {
   private:
    VotingRoundImpl(const std::shared_ptr<Grandpa> &grandpa,
                    const GrandpaConfig &config,
                    std::shared_ptr<AuthorityManager> authority_manager,
                    std::shared_ptr<Environment> env,
                    std::shared_ptr<VoteCryptoProvider> vote_crypto_provider,
                    std::shared_ptr<VoteTracker> prevotes,
                    std::shared_ptr<VoteTracker> precommits,
                    std::shared_ptr<VoteGraph> vote_graph,
                    std::shared_ptr<Clock> clock,
                    std::shared_ptr<libp2p::basic::Scheduler> scheduler);

   protected:
    // This ctor is needed only for tests purposes
    VotingRoundImpl() : round_number_{}, duration_{} {}

   public:
    VotingRoundImpl(
        const std::shared_ptr<Grandpa> &grandpa,
        const GrandpaConfig &config,
        const std::shared_ptr<AuthorityManager> authority_manager,
        const std::shared_ptr<Environment> &env,
        const std::shared_ptr<VoteCryptoProvider> &vote_crypto_provider,
        const std::shared_ptr<VoteTracker> &prevotes,
        const std::shared_ptr<VoteTracker> &precommits,
        const std::shared_ptr<VoteGraph> &vote_graph,
        const std::shared_ptr<Clock> &clock,
        const std::shared_ptr<libp2p::basic::Scheduler> &scheduler,
        const MovableRoundState &round_state);

    VotingRoundImpl(
        const std::shared_ptr<Grandpa> &grandpa,
        const GrandpaConfig &config,
        const std::shared_ptr<AuthorityManager> authority_manager,
        const std::shared_ptr<Environment> &env,
        const std::shared_ptr<VoteCryptoProvider> &vote_crypto_provider,
        const std::shared_ptr<VoteTracker> &prevotes,
        const std::shared_ptr<VoteTracker> &precommits,
        const std::shared_ptr<VoteGraph> &vote_graph,
        const std::shared_ptr<Clock> &clock,
        const std::shared_ptr<libp2p::basic::Scheduler> &scheduler,
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
    void doCommit() override;

    // Handlers of incoming messages

    outcome::result<void> applyJustification(
        const BlockInfo &block_info,
        const GrandpaJustification &justification) override;

    /**
     * Invoked when we received a primary propose for the current round
     * Basically method just checks if received propose was produced by the
     * primary and if so, it is stored in primary_vote_ field
     */
    void onProposal(const SignedMessage &proposal,
                    Propagation propagation) override;

    /**
     * Triggered when we receive prevote for current round
     * \param prevote is stored in prevote tracker and vote graph
     * Then we try to update prevote ghost (\see updatePrevoteGhost) and round
     * state (\see update)
     * @returns true if inner state has changed
     */
    bool onPrevote(const SignedMessage &prevote,
                   Propagation propagation) override;

    /**
     * Triggered when we receive precommit for the current round
     * \param precommit is stored in precommit tracker and vote graph
     * Then we try to update round state and finalize
     * @returns true if inner state has changed
     */
    bool onPrecommit(const SignedMessage &precommit,
                     Propagation propagation) override;

    /**
     * Updates inner state if something (see params) was changed since last call
     * @param is_previous_round_changed is true if previous round is changed
     * @param is_prevotes_changed is true if new prevote was accepted
     * @param is_precommits_changed is true if new precommits was accepted
     * @return true if finalized block was changed during update
     */
    void update(IsPreviousRoundChanged is_previous_round_changed,
                IsPrevotesChanged is_prevotes_changed,
                IsPrecommitsChanged is_precommits_changed) override;

    /**
     * @returns previous known round for current
     */
    std::shared_ptr<VotingRound> getPreviousRound() const override {
      return previous_round_;
    };

    /**
     * Removes previous round to limit chain of rounds
     */
    void forgetPreviousRound() override {
      previous_round_.reset();
    }

    /**
     * Checks if current round is completable and finalized block differs from
     * the last round's finalized block. If so fin message is broadcasted to the
     * network
     */
    void attemptToFinalizeRound() override;

    // Catch-up actions

    void doCatchUpResponse(const libp2p::peer::PeerId &peer_id) override;

    // Getters

    RoundNumber roundNumber() const override;
    VoterSetId voterSetId() const override;

    /**
     * Round is completable when we have block (stored in
     * current_state_.finalized) for which we have supermajority on both
     * prevotes and precommits
     */
    bool completable() const override;

    /**
     * Last finalized block
     * @return Block finalized in previous round (when current one was created)
     */
    BlockInfo lastFinalizedBlock() const override {
      return last_finalized_block_;
    }

    /**
     * Best block from descendants of previous round best-final-candidate
     * @see spec: Best-PreVote-Candidate
     */
    BlockInfo bestPrevoteCandidate() override;

    /**
     * Block what has precommit supermajority.
     * Should be descendant or equal of Best-PreVote-Candidate
     * @see spec:
     * [Best-Final-Candidate](https://spec.polkadot.network/develop/#algo-grandpa-best-candidate)
     * @endlink
     * @see spec:
     * [Ghost-Function](https://spec.polkadot.network/develop/#algo-grandpa-ghost)
     * @endlink
     */
    BlockInfo bestFinalCandidate() override;

    /**
     * The block, which is being finalized during this round
     */
    const std::optional<BlockInfo> &finalizedBlock() const override {
      return finalized_;
    };

    /**
     * @return state containing round number, last finalized block, votes, and
     * finalized block for this voting round
     */
    MovableRoundState state() const override;

    void sendNeighborMessage();

   private:
    /// Check if peer \param id is primary
    bool isPrimary(const Id &id) const;

    /// Triggered when we receive {@param vote} for the current peer
    template <typename T>
    outcome::result<void> onSigned(const SignedMessage &vote);

    /**
     * Invoked during each onSingedPrevote.
     * Updates current round's grandpa ghost. New grandpa-ghost is the highest
     * block with supermajority of prevotes
     * @return true if prevote ghost was updated
     */
    bool updateGrandpaGhost();

    /**
     * Invoked during each onSingedPrecommit.
     * @return true if estimate was updated
     */
    bool updateEstimate();

    /**
     * Prepare prevote justifications for provided estimate using provided votes
     * @param estimate estimate that we need to prepare justification for
     * @param votes votes that correspond to provided estimate
     * @return signed prevotes obtained from estimate and votes
     */
    std::vector<SignedPrevote> getPrevoteJustification(
        const BlockInfo &estimate, const std::vector<VoteVariant> &votes) const;

    /**
     * Prepare precommit justifications for provided estimate using provided
     * votes
     * @param precommits precommits that we need to prepare justification for
     * @param votes votes that correspond to provided precommits
     * @return signed precommits obtained from estimate and votes
     */
    std::vector<SignedPrecommit> getPrecommitJustification(
        const BlockInfo &precommits,
        const std::vector<VoteVariant> &votes) const;

    /**
     * Checks if received vote has valid justification precommit
     * @param vote - block for which justification is provided
     * @param justification - justification provided for checking
     * @return success of error
     */
    outcome::result<void> validatePrecommitJustification(
        const BlockInfo &vote, const GrandpaJustification &justification) const;

    void sendProposal(const PrimaryPropose &primary_proposal);
    void sendPrevote(const Prevote &prevote);
    void sendPrecommit(const Precommit &precommit);
    void pending();

    std::shared_ptr<VoterSet> voter_set_;
    const RoundNumber round_number_;
    std::shared_ptr<VotingRound> previous_round_;

    const Duration duration_;  // length of round
    bool isPrimary_ = false;
    size_t threshold_;                      // supermajority threshold
    const std::optional<Id> id_;            // id of current peer
    std::chrono::milliseconds start_time_;  // time of start round to play

    std::weak_ptr<Grandpa> grandpa_;
    std::shared_ptr<AuthorityManager> authority_manager_;
    std::shared_ptr<const primitives::AuthorityList> authorities_;
    std::shared_ptr<Environment> env_;
    std::shared_ptr<VoteCryptoProvider> vote_crypto_provider_;
    std::shared_ptr<VoteGraph> graph_;
    std::shared_ptr<Clock> clock_;
    std::shared_ptr<libp2p::basic::Scheduler> scheduler_;

    std::function<void()> on_complete_handler_;

    // Note: Pending interval must be longer than total voting time:
    //  2*Duration + 2*Duration + Gap
    // Spec says to send at least once per five minutes.
    // Substrate sends at least once per two minutes.
    const std::chrono::milliseconds pending_interval_ =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::max<Clock::Duration>(duration_ * 10,
                                      std::chrono::seconds(12)));

    Stage stage_ = Stage::INIT;

    std::shared_ptr<VoteTracker> prevotes_;
    std::shared_ptr<VoteTracker> precommits_;

    // equivocators arrays. Index in vector corresponds to the index of voter in
    // voter set, value corresponds to the weight of the voter
    std::vector<bool> prevote_equivocators_;
    std::vector<bool> precommit_equivocators_;

    // Proposed primary vote.
    // It's best final candidate of previous round
    std::optional<BlockInfo> primary_vote_;

    // Our vote at prevote stage.
    // It's the deepest descendant of primary vote (or last finalized)
    std::optional<BlockInfo> prevote_;

    // Our vote at precommit stage. Setting once.
    // It's the deepest descendant of best prevote candidate with prevote
    // supermajority
    std::optional<BlockInfo> precommit_;

    // Last finalized block at the moment of round is cteated
    BlockInfo last_finalized_block_;

    // Prevote ghost. Updating by each prevote.
    // It's deepest descendant of primary vote (or last finalized) with prevote
    // supermajority Is't also the best prevote candidate
    std::optional<BlockInfo> prevote_ghost_;

    std::optional<BlockInfo> estimate_;
    std::optional<BlockInfo> finalized_;

    libp2p::basic::Scheduler::Handle stage_timer_handle_;
    libp2p::basic::Scheduler::Handle pending_timer_handle_;

    log::Logger logger_ = log::createLogger("VotingRound", "voting_round");

    bool completable_ = false;
  };
}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CONSENSUS_GRANDPA_VOTINGROUNDIMPL
