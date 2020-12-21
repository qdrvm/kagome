/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_GRANDPA_GRANDPAIMPL
#define KAGOME_CONSENSUS_GRANDPA_GRANDPAIMPL

#include "consensus/grandpa/grandpa.hpp"
#include "consensus/grandpa/grandpa_observer.hpp"

#include "application/app_state_manager.hpp"
#include "blockchain/block_tree.hpp"
#include "common/logger.hpp"
#include "consensus/authority/authority_manager.hpp"
#include "consensus/babe/babe.hpp"
#include "consensus/grandpa/environment.hpp"
#include "consensus/grandpa/impl/voting_round_impl.hpp"
#include "consensus/grandpa/movable_round_state.hpp"
#include "consensus/grandpa/voter_set.hpp"
#include "crypto/ed25519_provider.hpp"
#include "crypto/hasher.hpp"
#include "network/gossiper.hpp"
#include "runtime/grandpa_api.hpp"
#include "storage/buffer_map_types.hpp"

namespace kagome::consensus::grandpa {

  class GrandpaImpl : public Grandpa,
                      public GrandpaObserver,
                      public std::enable_shared_from_this<GrandpaImpl> {
   public:
    ~GrandpaImpl() override = default;

    GrandpaImpl(std::shared_ptr<application::AppStateManager> app_state_manager,
                std::shared_ptr<Environment> environment,
                std::shared_ptr<storage::BufferStorage> storage,
                std::shared_ptr<crypto::Ed25519Provider> crypto_provider,
                std::shared_ptr<runtime::GrandpaApi> grandpa_api,
                const crypto::Ed25519Keypair &keypair,
                std::shared_ptr<Clock> clock,
                std::shared_ptr<boost::asio::io_context> io_context,
                std::shared_ptr<authority::AuthorityManager> authority_manager,
                std::shared_ptr<consensus::Babe> babe);

    /** @see AppStateManager::takeControl */
    bool prepare();

    /** @see AppStateManager::takeControl */
    bool start();

    /** @see AppStateManager::takeControl */
    void stop();

    // Catch-up methods

    void onCatchUpRequest(const libp2p::peer::PeerId &peer_id,
                          const network::CatchUpRequest &msg) override;

    void onCatchUpResponse(const libp2p::peer::PeerId &peer_id,
                           const network::CatchUpResponse &msg) override;

    // Voting methods

    void onVoteMessage(const libp2p::peer::PeerId &peer_id,
                       const VoteMessage &msg) override;

    void onFinalize(const libp2p::peer::PeerId &peer_id,
                    const Fin &fin) override;

    // Round processing method

    void executeNextRound() override;

   private:
    std::shared_ptr<VotingRound> selectRound(RoundNumber round_number);
    outcome::result<std::shared_ptr<VoterSet>> getVoters() const;
    outcome::result<MovableRoundState> getLastCompletedRound() const;

    std::shared_ptr<VotingRound> makeInitialRound(
        const MovableRoundState &round_state);

    std::shared_ptr<VotingRound> makeNextRound(
        const std::shared_ptr<VotingRound> &previous_round);

    void onCompletedRound(outcome::result<MovableRoundState> round_state_res);

    // Note: Duration value was gotten from substrate
    // https://github.com/paritytech/substrate/blob/efbac7be80c6e8988a25339061078d3e300f132d/bin/node-template/node/src/service.rs#L166
    // Perhaps, 333ms is not enough for normal communication during the round
    const Clock::Duration round_time_factor_ = std::chrono::milliseconds(333);

    std::shared_ptr<VotingRound> previous_round_;
    std::shared_ptr<VotingRound> current_round_;

    std::shared_ptr<application::AppStateManager> app_state_manager_;
    std::shared_ptr<Environment> environment_;
    std::shared_ptr<storage::BufferStorage> storage_;
    std::shared_ptr<crypto::Ed25519Provider> crypto_provider_;
    std::shared_ptr<runtime::GrandpaApi> grandpa_api_;
    crypto::Ed25519Keypair keypair_;
    std::shared_ptr<Clock> clock_;
    std::shared_ptr<boost::asio::io_context> io_context_;
    std::shared_ptr<authority::AuthorityManager> authority_manager_;

    bool is_ready_ = false;
    std::shared_ptr<consensus::Babe> babe_;

    const Clock::Duration catch_up_request_suppression_duration_ =
        std::chrono::seconds(15);
    Clock::TimePoint catch_up_request_suppressed_until_;

    common::Logger logger_ = common::createLogger("Grandpa");
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CONSENSUS_GRANDPA_GRANDPAIMPL
