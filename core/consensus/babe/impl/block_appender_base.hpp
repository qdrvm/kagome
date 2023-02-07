/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BLOCK_APPENDER_BASE_HPP
#define KAGOME_BLOCK_APPENDER_BASE_HPP

#include "log/logger.hpp"
#include "outcome/outcome.hpp"
#include "primitives/block_data.hpp"

namespace kagome::blockchain {
  class BlockTree;
  class DigestTracker;
}  // namespace kagome::blockchain

namespace kagome::crypto {
  class Hasher;
}

namespace kagome::consensus::grandpa {
  class Environment;
}

namespace kagome::consensus::babe {

  class BlockValidator;
  class BabeConfigRepository;
  class ConsistencyKeeper;
  class ConsistencyGuard;
  class BabeUtil;

  /**
   * Common logic for adding a new block to the blockchain
   */
  class BlockAppenderBase {
   public:
    BlockAppenderBase(std::shared_ptr<ConsistencyKeeper> consistency_keeper,
                      std::shared_ptr<blockchain::BlockTree> block_tree,
                      std::shared_ptr<blockchain::DigestTracker> digest_tracker,
                      std::shared_ptr<BabeConfigRepository> babe_config_repo,
                      std::shared_ptr<BlockValidator> block_validator,
                      std::shared_ptr<grandpa::Environment> grandpa_environment,
                      std::shared_ptr<BabeUtil> babe_util,
                      std::shared_ptr<crypto::Hasher> hasher);

    primitives::BlockContext makeBlockContext(
        primitives::BlockHeader const &header) const;

    outcome::result<void> applyJustifications(
        const primitives::BlockInfo &block_info,
        const std::optional<primitives::Justification> &new_justification);

    outcome::result<ConsistencyGuard> observeDigestsAndValidateHeader(
        primitives::Block const &block,
        primitives::BlockContext const &context);

   private:
    log::Logger logger_;

    // TODO(Harrm): describe why
    //  Justifications stored for future application (why?)
    std::map<primitives::BlockInfo, primitives::Justification>
        postponed_justifications_;

    std::shared_ptr<ConsistencyKeeper> consistency_keeper_;
    std::shared_ptr<blockchain::BlockTree> block_tree_;
    std::shared_ptr<blockchain::DigestTracker> digest_tracker_;
    std::shared_ptr<BabeConfigRepository> babe_config_repo_;
    std::shared_ptr<BlockValidator> block_validator_;
    std::shared_ptr<grandpa::Environment> grandpa_environment_;
    std::shared_ptr<BabeUtil> babe_util_;
    std::shared_ptr<crypto::Hasher> hasher_;
  };

}  // namespace kagome::consensus::babe

#endif  // KAGOME_BLOCK_APPENDER_BASE_HPP
