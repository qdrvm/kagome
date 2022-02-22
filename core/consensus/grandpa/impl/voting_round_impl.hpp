/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_GRANDPA_VOTINGROUNDIMPL
#define KAGOME_CONSENSUS_GRANDPA_VOTINGROUNDIMPL

#include "consensus/grandpa/voting_round.hpp"

#include <boost/asio/basic_waitable_timer.hpp>
#include <boost/signals2.hpp>
#include <libp2p/basic/scheduler.hpp>

#include "consensus/authority/authority_manager.hpp"
#include "consensus/grandpa/environment.hpp"
#include "consensus/grandpa/grandpa_config.hpp"
#include "consensus/grandpa/movable_round_state.hpp"
#include "consensus/grandpa/structs.hpp"
#include "consensus/grandpa/vote_crypto_provider.hpp"
#include "consensus/grandpa/vote_graph.hpp"
#include "consensus/grandpa/vote_tracker.hpp"
#include "log/logger.hpp"

namespace kagome::consensus::grandpa {

  class Grandpa;

  class VotingRoundImpl : public VotingRound {
   private:
    VotingRoundImpl(
        const std::shared_ptr<Grandpa> &grandpa,
        const GrandpaConfig &config,
        std::shared_ptr<authority::AuthorityManager> authority_manager,
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
        const std::shared_ptr<authority::AuthorityManager> authority_manager,
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
        const std::shared_ptr<authority::AuthorityManager> authority_manager,
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
    void update(bool is_previous_round_changed,
                bool is_prevotes_changed,
                bool is_precommits_changed) override;

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
    MembershipCounter voterSetId() const override;

    bool completable() const override;
    bool finalizable() const override;

    BlockInfo lastFinalizedBlock() const override {
      return last_finalized_block_;
    }
    BlockInfo bestPrevoteCandidate() override;
    BlockInfo bestFinalCandidate() override;
    std::optional<BlockInfo> finalizedBlock() const override {
      return finalized_;
    };

    MovableRoundState state() const override;

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

    /// prepare prevote justification of \param estimate over the provided
    /// \param votes
    std::vector<SignedPrevote> getPrevoteJustification(
        const BlockInfo &estimate, const std::vector<VoteVariant> &votes) const;

    /// prepare precommit justification of \param estimate over the provided
    /// \param votes
    std::vector<SignedPrecommit> getPrecommitJustification(
        const BlockInfo &precommits,
        const std::vector<VoteVariant> &votes) const;

    /// Check if received \param vote has valid \param justification precommit
    outcome::result<void> validatePrecommitJustification(
        const BlockInfo &vote, const GrandpaJustification &justification) const;

    void sendNeighborMessage();
    void sendProposal(const PrimaryPropose &primary_proposal);
    void sendPrevote(const Prevote &prevote);
    void sendPrecommit(const Precommit &precommit);
    void sendCommit();
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
    std::shared_ptr<authority::AuthorityManager> authority_manager_;
    std::shared_ptr<const primitives::AuthorityList> authorities_;
    std::shared_ptr<Environment> env_;
    std::shared_ptr<VoteCryptoProvider> vote_crypto_provider_;
    std::shared_ptr<VoteGraph> graph_;
    std::shared_ptr<Clock> clock_;
    std::shared_ptr<libp2p::basic::Scheduler> scheduler_;

    std::function<void()> on_complete_handler_;

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
    // It's deepest descendant of primary vote (or last finalized)
    std::optional<BlockInfo> prevote_;

    // Our vote at precommit stage. Setting once.
    // It's deepest descendant of best prevote candidate with prevote
    // supermajority
    std::optional<BlockInfo> precommit_;

    // Last finalized block at the moment of round is cteated
    BlockInfo last_finalized_block_;

    // Prevote ghost. Updating by each prevote.
    // It's deepest descendant of primary vote (or last finalized) with prevote
    // supermajority Is't also the best prevote candidate
    std::optional<BlockInfo> prevote_ghost_;

    std::optional<BlockInfo> estimate_;

    std::optional<BlockInfo> best_final_candidate_;
    std::optional<BlockInfo> finalized_;

    libp2p::basic::Scheduler::Handle stage_timer_handle_;
    libp2p::basic::Scheduler::Handle pending_timer_handle_;

    log::Logger logger_ = log::createLogger("VotingRound", "voting_round");

    bool completable_ = false;
  };
}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CONSENSUS_GRANDPA_VOTINGROUNDIMPL
