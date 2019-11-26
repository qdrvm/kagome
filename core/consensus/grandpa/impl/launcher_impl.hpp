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
#include "consensus/grandpa/gossiper.hpp"
#include "consensus/grandpa/launcher.hpp"
#include "consensus/grandpa/vote_crypto_provider.hpp"
#include "consensus/grandpa/voter_set.hpp"
#include "consensus/grandpa/voting_round.hpp"
#include "storage/trie/trie_db.hpp"

namespace kagome::consensus::grandpa {

  class LauncherImpl : public Launcher {
   public:
    ~LauncherImpl() override = default;

    LauncherImpl(std::shared_ptr<Environment> environment,
                 std::shared_ptr<storage::trie::TrieDb> storage,
                 std::shared_ptr<VoteCryptoProvider> vote_crypto_provider,
                 Id id,
                 std::shared_ptr<Clock> clock,
                 std::shared_ptr<boost::asio::io_context> io_context);

    void start() override;

    void onVoteMessage(const VoteMessage &msg) override;

    void executeNextRound();

   private:
    outcome::result<std::shared_ptr<VoterSet>> getVoters() const;
    outcome::result<CompletedRound> getLastRoundNumber() const;

    std::shared_ptr<VotingRound> current_round_;
    std::shared_ptr<Environment> environment_;
    std::shared_ptr<storage::trie::TrieDb> storage_;
    std::shared_ptr<VoteCryptoProvider> vote_crypto_provider_;
    Id id_;
    std::shared_ptr<Clock> clock_;
    std::shared_ptr<boost::asio::io_context> io_context_;

    common::Logger logger_ = common::createLogger("Grandpa launcher");
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CORE_CONSENSUS_GRANDPA_IMPL_LAUNCHERIMPL_HPP
