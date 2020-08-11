/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CONSENSUS_GRANDPA_IMPL_GRANDPAIMPL
#define KAGOME_CORE_CONSENSUS_GRANDPA_IMPL_GRANDPAIMPL

#include "consensus/grandpa/grandpa.hpp"

#include <consensus/authority/authority_manager.hpp>
#include <optional>

#include "application/app_state_manager.hpp"
#include "blockchain/block_tree.hpp"
#include "common/logger.hpp"
#include "consensus/grandpa/completed_round.hpp"
#include "consensus/grandpa/environment.hpp"
#include "consensus/grandpa/impl/voting_round_impl_2.hpp"
#include "consensus/grandpa/voter_set.hpp"
#include "crypto/ed25519_provider.hpp"
#include "crypto/hasher.hpp"
#include "network/gossiper.hpp"
#include "runtime/grandpa_api.hpp"
#include "storage/buffer_map_types.hpp"

namespace kagome::consensus::grandpa {

  class GrandpaImpl2 : public Grandpa,
                       public std::enable_shared_from_this<GrandpaImpl2> {
   public:
    ~GrandpaImpl2() override = default;

    GrandpaImpl2(
        std::shared_ptr<application::AppStateManager> app_state_manager,
        std::shared_ptr<Environment> environment,
        std::shared_ptr<storage::BufferStorage> storage,
        std::shared_ptr<crypto::ED25519Provider> crypto_provider,
        std::shared_ptr<runtime::GrandpaApi> grandpa_api,
        const crypto::ED25519Keypair &keypair,
        std::shared_ptr<Clock> clock,
        std::shared_ptr<boost::asio::io_context> io_context,
        std::shared_ptr<authority::AuthorityManager> authority_manager);

    bool prepare() override;
    bool start() override;
    void stop() override;

    std::shared_ptr<VotingRoundImpl2> previous_round;
    std::shared_ptr<VotingRoundImpl2> current_round;

    std::shared_ptr<VotingRoundImpl2> makeInitialRound(
        RoundNumber previous_round_number,
        std::shared_ptr<RoundState> previous_round_state);

    std::shared_ptr<VotingRoundImpl2> makeNextRound(
        const std::shared_ptr<VotingRoundImpl2> &previous_round_state);

    /**
     * TODO (PRE-371): kamilsa remove this method when grandpa issue resolved
     *
     * Start timer which constantly checks if grandpa rounds are running. If not
     * relaunches grandpa
     */
    void readinessCheck();

    void onVoteMessage(const VoteMessage &msg) override;

    void onFinalize(const Fin &f) override;

    void executeNextRound();

   private:
    void onCompletedRound(outcome::result<CompletedRound> completed_round_res);

    outcome::result<std::shared_ptr<VoterSet>> getVoters() const;
    outcome::result<CompletedRound> getLastCompletedRound() const;

    std::shared_ptr<application::AppStateManager> app_state_manager_;
    std::shared_ptr<Environment> environment_;
    std::shared_ptr<storage::BufferStorage> storage_;
    std::shared_ptr<crypto::ED25519Provider> crypto_provider_;
    std::shared_ptr<runtime::GrandpaApi> grandpa_api_;
    crypto::ED25519Keypair keypair_;
    std::shared_ptr<Clock> clock_;
    std::shared_ptr<boost::asio::io_context> io_context_;
    std::shared_ptr<authority::AuthorityManager> authority_manager_;
    Timer liveness_checker_;

    common::Logger logger_ = common::createLogger("Grandpa");
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CORE_CONSENSUS_GRANDPA_IMPL_GRANDPAIMPL
