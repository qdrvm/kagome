/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CONSENSUS_GRANDPA_IMPL_LAUNCHERIMPL_HPP
#define KAGOME_CORE_CONSENSUS_GRANDPA_IMPL_LAUNCHERIMPL_HPP

#include "blockchain/block_tree.hpp"
#include "common/logger.hpp"
#include "consensus/grandpa/completed_round.hpp"
#include "consensus/grandpa/environment.hpp"
#include "consensus/grandpa/launcher.hpp"
#include "consensus/grandpa/voter_set.hpp"
#include "consensus/grandpa/voting_round.hpp"
#include "crypto/ed25519_provider.hpp"
#include "network/gossiper.hpp"
#include "runtime/grandpa.hpp"
#include "storage/buffer_map_types.hpp"

namespace kagome::consensus::grandpa {

  class LauncherImpl : public Launcher,
                       public std::enable_shared_from_this<LauncherImpl> {
   public:
    ~LauncherImpl() override = default;

    LauncherImpl(std::shared_ptr<Environment> environment,
                 std::shared_ptr<storage::BufferStorage> storage,
                 std::shared_ptr<crypto::ED25519Provider> crypto_provider,
                 std::shared_ptr<runtime::Grandpa> grandpa_api,
                 const crypto::ED25519Keypair &keypair,
                 std::shared_ptr<Clock> clock,
                 std::shared_ptr<boost::asio::io_context> io_context);

    void start() override;

    /**
     * TODO (PRE-371): kamilsa remove this method when grandpa issue resolved
     *
     * Start timer which constantly checks if grandpa rounds are running. If not
     * relaunches grandpa
     */
    void startLivenessChecker();

    void onVoteMessage(const VoteMessage &msg) override;

    void onFinalize(const Fin &f) override;

    void executeNextRound();

   private:
    outcome::result<std::shared_ptr<VoterSet>> getVoters() const;
    outcome::result<CompletedRound> getLastCompletedRound() const;

    std::shared_ptr<VotingRound> current_round_;

    std::shared_ptr<Environment> environment_;
    std::shared_ptr<storage::BufferStorage> storage_;
    std::shared_ptr<crypto::ED25519Provider> crypto_provider_;
    std::shared_ptr<runtime::Grandpa> grandpa_api_;
    crypto::ED25519Keypair keypair_;
    std::shared_ptr<Clock> clock_;
    std::shared_ptr<boost::asio::io_context> io_context_;
    Timer liveness_checker_;

    common::Logger logger_ = common::createLogger("Grandpa launcher");
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CORE_CONSENSUS_GRANDPA_IMPL_LAUNCHERIMPL_HPP
