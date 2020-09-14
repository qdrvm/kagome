/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CONSENSUS_GRANDPA_IMPL_GRANDPAIMPL
#define KAGOME_CORE_CONSENSUS_GRANDPA_IMPL_GRANDPAIMPL

#include <consensus/grandpa/movable_round_state.hpp>
#include "consensus/grandpa/grandpa.hpp"

#include "application/app_state_manager.hpp"
#include "blockchain/block_tree.hpp"
#include "common/logger.hpp"
#include "consensus/authority/authority_manager.hpp"
#include "consensus/grandpa/completed_round.hpp"
#include "consensus/grandpa/environment.hpp"
#include "consensus/grandpa/impl/voting_round_impl.hpp"
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
                std::shared_ptr<crypto::ED25519Provider> crypto_provider,
                std::shared_ptr<runtime::GrandpaApi> grandpa_api,
                const crypto::ED25519Keypair &keypair,
                std::shared_ptr<Clock> clock,
                std::shared_ptr<boost::asio::io_context> io_context,
                std::shared_ptr<authority::AuthorityManager> authority_manager);

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

    void onFinalize(const libp2p::peer::PeerId &peer_id, const Fin &fin) override;

    // Round processing method

    void executeNextRound() override;

   private:
    std::shared_ptr<VotingRound> selectRound(RoundNumber round_number);
    outcome::result<std::shared_ptr<VoterSet>> getVoters() const;
    outcome::result<MovableRoundState> getLastCompletedRound() const;

    std::shared_ptr<VotingRound> makeInitialRound(
        const MovableRoundState& round_state);

    std::shared_ptr<VotingRound> makeNextRound(
        const std::shared_ptr<VotingRound> &previous_round);

    void onCompletedRound(outcome::result<MovableRoundState> round_state_res);

    std::shared_ptr<VotingRound> previous_round_;
    std::shared_ptr<VotingRound> current_round_;
    std::shared_ptr<VotingRound> last_finalised_round_;

    std::shared_ptr<application::AppStateManager> app_state_manager_;
    std::shared_ptr<Environment> environment_;
    std::shared_ptr<storage::BufferStorage> storage_;
    std::shared_ptr<crypto::ED25519Provider> crypto_provider_;
    std::shared_ptr<runtime::GrandpaApi> grandpa_api_;
    crypto::ED25519Keypair keypair_;
    std::shared_ptr<Clock> clock_;
    std::shared_ptr<boost::asio::io_context> io_context_;
    std::shared_ptr<authority::AuthorityManager> authority_manager_;

    common::Logger logger_ = common::createLogger("Grandpa");
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CORE_CONSENSUS_GRANDPA_IMPL_GRANDPAIMPL
