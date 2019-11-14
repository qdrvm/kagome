/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CONSENSUS_GRANDPA_IMPL_LAUNCHERIMPL_HPP
#define KAGOME_CORE_CONSENSUS_GRANDPA_IMPL_LAUNCHERIMPL_HPP

#include <boost/smart_ptr/atomic_shared_ptr.hpp>
#include "blockchain/block_tree.hpp"
#include "consensus/grandpa/chain.hpp"
#include "consensus/grandpa/completed_round.hpp"
#include "consensus/grandpa/gossiper.hpp"
#include "consensus/grandpa/launcher.hpp"
#include "consensus/grandpa/voting_round.hpp"
#include "crypto/ed25519_provider.hpp"
#include "runtime/core.hpp"
#include "storage/trie/trie_db.hpp"

namespace kagome::consensus::grandpa {

  class LauncherImpl : public Launcher {
   public:
    ~LauncherImpl() override = default;

    void start() override;

    void onVoteMessage(
        const kagome::consensus::grandpa::VoteMessage &msg) override;

    void executeNextRound();

   private:
    std::shared_ptr<VoterSet> getVoters() const;
    CompletedRound getLastRoundNumber() const;

    std::shared_ptr<VotingRound> current_round_;
    std::shared_ptr<blockchain::BlockTree> block_tree_;
    std::shared_ptr<storage::PersistentBufferMap> storage_;
    std::shared_ptr<runtime::Core> core_;
    crypto::ED25519Keypair keypair_;
    std::shared_ptr<Chain> chain_;
    std::shared_ptr<Gossiper> gossiper_;
    std::shared_ptr<crypto::ED25519Provider> ed_provider_;
    std::shared_ptr<Clock> clock_;
    std::shared_ptr<boost::asio::io_context> io_context_;
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CORE_CONSENSUS_GRANDPA_IMPL_LAUNCHERIMPL_HPP
