/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_APPLICATION_RECOVERYMODE
#define KAGOME_APPLICATION_RECOVERYMODE

#include "application/mode.hpp"

#include <memory>

#include "log/logger.hpp"

namespace kagome::application {
  class AppConfiguration;
}

namespace kagome::blockchain {
  class BlockStorage;
  class BlockHeaderRepository;
  class BlockTree;
}  // namespace kagome::blockchain

namespace kagome::consensus::grandpa {
  class AuthorityManager;
}

namespace kagome::storage {
  class SpacedStorage;
}

namespace kagome::storage::trie {
  class TrieStorage;
}

namespace kagome::application::mode {

  /// Mode for recover state to provided block
  class RecoveryMode final : public Mode {
   public:
    RecoveryMode(
        const application::AppConfiguration &app_config,
        std::shared_ptr<storage::SpacedStorage> spaced_storage,
        std::shared_ptr<blockchain::BlockStorage> storage,
        std::shared_ptr<blockchain::BlockHeaderRepository> header_repo,
        std::shared_ptr<const storage::trie::TrieStorage> trie_storage,
        std::shared_ptr<consensus::grandpa::AuthorityManager> authority_manager,
        std::shared_ptr<blockchain::BlockTree> block_tree);

    int run() const override;

   private:
    const application::AppConfiguration &app_config_;
    std::shared_ptr<storage::SpacedStorage> spaced_storage_;
    std::shared_ptr<blockchain::BlockStorage> storage_;
    std::shared_ptr<blockchain::BlockHeaderRepository> header_repo_;
    std::shared_ptr<const storage::trie::TrieStorage> trie_storage_;
    std::shared_ptr<consensus::grandpa::AuthorityManager> authority_manager_;
    std::shared_ptr<blockchain::BlockTree> block_tree_;

    log::Logger log_;
  };

}  // namespace kagome::application::mode

#endif  // KAGOME_APPLICATION_RECOVERYMODE
