/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef KAGOME_CORE_CONSENSUS_GRANDPA_IMPL_LAUNCHERIMPL_HPP
#define KAGOME_CORE_CONSENSUS_GRANDPA_IMPL_LAUNCHERIMPL_HPP

#include "blockchain/block_tree.hpp"
#include "common/logger.hpp"
#include "consensus/grandpa/completed_round.hpp"
#include "consensus/grandpa/environment.hpp"
#include "consensus/grandpa/gossiper.hpp"
#include "consensus/grandpa/launcher.hpp"
#include "consensus/grandpa/voter_set.hpp"
#include "consensus/grandpa/voting_round.hpp"
#include "crypto/ed25519_provider.hpp"
#include "storage/buffer_map_types.hpp"

namespace kagome::consensus::grandpa {

  class LauncherImpl : public Launcher,
                       public std::enable_shared_from_this<LauncherImpl> {
   public:
    ~LauncherImpl() override = default;

    LauncherImpl(std::shared_ptr<Environment> environment,
                 std::shared_ptr<storage::BufferStorage> storage,
                 std::shared_ptr<crypto::ED25519Provider> crypto_provider,
                 const crypto::ED25519Keypair &keypair,
                 std::shared_ptr<Clock> clock,
                 std::shared_ptr<boost::asio::io_context> io_context);

    void start() override;

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
    crypto::ED25519Keypair keypair_;
    std::shared_ptr<Clock> clock_;
    std::shared_ptr<boost::asio::io_context> io_context_;

    common::Logger logger_ = common::createLogger("Grandpa launcher");
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CORE_CONSENSUS_GRANDPA_IMPL_LAUNCHERIMPL_HPP
