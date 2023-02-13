/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_BABE_BLOCKAPPENDERIMPL
#define KAGOME_CONSENSUS_BABE_BLOCKAPPENDERIMPL

#include "consensus/babe/block_appender.hpp"

#include <libp2p/peer/peer_id.hpp>

#include "log/logger.hpp"
#include "primitives/babe_configuration.hpp"
#include "primitives/block_header.hpp"

namespace kagome::consensus::babe {
  class BabeConfigRepository;
  class BabeUtil;
  class BlockValidator;
  class ConsistencyKeeper;
}  // namespace kagome::consensus::babe

namespace kagome::consensus::grandpa {
  class Environment;
}

namespace kagome::blockchain {
  class BlockTree;
  class DigestTracker;
}  // namespace kagome::blockchain

namespace kagome::crypto {
  class Hasher;
}

namespace kagome::consensus::babe {
  class BlockAppenderImpl
      : public BlockAppender,
        public std::enable_shared_from_this<BlockAppenderImpl> {
   public:
    enum class Error { INVALID_BLOCK = 1, PARENT_NOT_FOUND };

    BlockAppenderImpl(std::shared_ptr<blockchain::BlockTree> block_tree,
                      std::shared_ptr<BabeConfigRepository> babe_config_repo,
                      std::shared_ptr<BlockValidator> block_validator,
                      std::shared_ptr<grandpa::Environment> grandpa_environment,
                      std::shared_ptr<crypto::Hasher> hasher,
                      std::shared_ptr<blockchain::DigestTracker> digest_tracker,
                      std::shared_ptr<BabeUtil> babe_util,
                      std::shared_ptr<ConsistencyKeeper> consistency_keeper);

    outcome::result<void> appendBlock(primitives::BlockData &&b) override;

    outcome::result<void> applyJustification(
        const primitives::BlockInfo &block_info,
        const primitives::Justification &justification) override;

   private:
    std::shared_ptr<blockchain::BlockTree> block_tree_;
    std::shared_ptr<BabeConfigRepository> babe_config_repo_;
    std::shared_ptr<BlockValidator> block_validator_;
    std::shared_ptr<grandpa::Environment> grandpa_environment_;
    std::shared_ptr<crypto::Hasher> hasher_;
    std::shared_ptr<blockchain::DigestTracker> digest_tracker_;
    std::shared_ptr<BabeUtil> babe_util_;
    std::shared_ptr<ConsistencyKeeper> consistency_keeper_;

    std::optional<primitives::BlockInfo> last_appended_;

    // Justification Store for Future Applying
    std::map<primitives::BlockInfo, primitives::Justification>
        postponed_justifications_;

    struct {
      std::chrono::high_resolution_clock::time_point time;
      primitives::BlockNumber block_number = 0;
    } speed_data_ = {};

    log::Logger logger_;
  };

}  // namespace kagome::consensus::babe

OUTCOME_HPP_DECLARE_ERROR(kagome::consensus::babe, BlockAppenderImpl::Error);

#endif  // KAGOME_CONSENSUS_BABE_BLOCKAPPENDERIMPL
