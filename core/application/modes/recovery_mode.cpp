/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/modes/recovery_mode.hpp"

#include <boost/assert.hpp>

#include "application/app_configuration.hpp"
#include "blockchain/impl/block_tree_impl.hpp"
#include "storage/predefined_keys.hpp"
#include "storage/spaced_storage.hpp"

namespace kagome::application::mode {

  RecoveryMode::RecoveryMode(
      const AppConfiguration &app_config,
      std::shared_ptr<storage::SpacedStorage> spaced_storage,
      std::shared_ptr<blockchain::BlockStorage> storage,
      std::shared_ptr<blockchain::BlockHeaderRepository> header_repo,
      std::shared_ptr<const storage::trie::TrieStorage> trie_storage,
      std::shared_ptr<consensus::grandpa::AuthorityManager> authority_manager,
      std::shared_ptr<blockchain::BlockTree> block_tree)
      : app_config_(app_config),
        spaced_storage_(std::move(spaced_storage)),
        storage_(std::move(storage)),
        header_repo_(std::move(header_repo)),
        trie_storage_(std::move(trie_storage)),
        authority_manager_(std::move(authority_manager)),
        block_tree_(std::move(block_tree)),
        log_(log::createLogger("RecoveryMode", "main")) {
    BOOST_ASSERT(spaced_storage_ != nullptr);
    BOOST_ASSERT(storage_ != nullptr);
    BOOST_ASSERT(header_repo_ != nullptr);
    BOOST_ASSERT(trie_storage_ != nullptr);
    BOOST_ASSERT(authority_manager_ != nullptr);
    BOOST_ASSERT(block_tree_ != nullptr);
  }

  int RecoveryMode::run() const {
    BOOST_ASSERT(app_config_.recoverState().has_value());
    auto res =
        blockchain::BlockTreeImpl::recover(app_config_.recoverState().value(),
                                           storage_,
                                           header_repo_,
                                           trie_storage_,
                                           block_tree_);

    spaced_storage_->getSpace(storage::Space::kDefault)
        ->remove(storage::kAuthorityManagerStateLookupKey("last"))
        .value();
    if (res.has_error()) {
      SL_ERROR(log_, "Recovery mode has failed: {}", res.error());
      log_->flush();
      return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
  }
}  // namespace kagome::application::mode
