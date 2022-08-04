/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_BLOCKAPPENDERIMPL
#define KAGOME_CONSENSUS_BLOCKAPPENDERIMPL

#include "consensus/babe/block_appender.hpp"

#include <libp2p/peer/peer_id.hpp>

#include "blockchain/block_tree.hpp"
#include "clock/timer.hpp"
#include "consensus/authority/authority_update_observer.hpp"
#include "consensus/babe/babe_util.hpp"
#include "consensus/grandpa/environment.hpp"
#include "consensus/validation/block_validator.hpp"
#include "crypto/hasher.hpp"
#include "log/logger.hpp"
#include "primitives/babe_configuration.hpp"
#include "primitives/block_header.hpp"

namespace kagome::consensus {
  namespace babe {
    class ConsistencyKeeper;
  }

  class BlockAppenderImpl
      : public BlockAppender,
        public std::enable_shared_from_this<BlockAppenderImpl> {
   public:
    enum class Error { INVALID_BLOCK = 1, PARENT_NOT_FOUND };

    BlockAppenderImpl(
        std::shared_ptr<blockchain::BlockTree> block_tree,
        std::shared_ptr<primitives::BabeConfiguration> configuration,
        std::shared_ptr<BlockValidator> block_validator,
        std::shared_ptr<grandpa::Environment> grandpa_environment,
        std::shared_ptr<crypto::Hasher> hasher,
        std::shared_ptr<authority::AuthorityUpdateObserver>
            authority_update_observer,
        std::shared_ptr<BabeUtil> babe_util,
        std::shared_ptr<babe::ConsistencyKeeper> consistency_keeper);

    outcome::result<void> appendBlock(primitives::BlockData &&b) override;

    outcome::result<void> applyJustification(
        const primitives::BlockInfo &block_info,
        const primitives::Justification &justification) override;

   private:
    std::shared_ptr<blockchain::BlockTree> block_tree_;
    std::shared_ptr<primitives::BabeConfiguration> babe_configuration_;
    std::shared_ptr<BlockValidator> block_validator_;
    std::shared_ptr<grandpa::Environment> grandpa_environment_;
    std::shared_ptr<crypto::Hasher> hasher_;
    std::shared_ptr<authority::AuthorityUpdateObserver>
        authority_update_observer_;
    std::shared_ptr<BabeUtil> babe_util_;
    std::shared_ptr<babe::ConsistencyKeeper> consistency_keeper_;

    // Justification Store for Future Applying
    std::map<primitives::BlockInfo, primitives::Justification> justifications_;

    log::Logger logger_;
  };

}  // namespace kagome::consensus

OUTCOME_HPP_DECLARE_ERROR(kagome::consensus, BlockAppenderImpl::Error);

#endif  // KAGOME_CONSENSUS_BLOCKAPPENDERIMPL
