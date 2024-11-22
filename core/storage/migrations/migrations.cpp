#include "storage/migrations/migrations.hpp"
#include <qtils/outcome.hpp>
#include <soralog/macro.hpp>
#include <unordered_set>

#include "blockchain/block_tree.hpp"
#include "blockchain/block_tree_error.hpp"
#include "common/blob.hpp"
#include "injector/application_injector.hpp"
#include "log/logger.hpp"
#include "primitives/common.hpp"
#include "storage/spaced_storage.hpp"
#include "storage/trie/trie_storage.hpp"
#include "storage/trie/trie_storage_backend.hpp"

namespace kagome::storage::migrations {

  outcome::result<void> separateTrieValues(
      const blockchain::BlockTree &block_tree,
      const trie::TrieStorage &trie_storage,
      SpacedStorage &storage) {
    auto logger = log::createLogger("Migration", "storage");
    SL_INFO(logger,
            "Begin trie storate migration to separate nodes and values");
    if (storage.getSpace(Space::kTrieValue)->cursor()->isValid()) {
      SL_INFO(logger,
              "Stop trie storate migration, trie values column is not empty "
              "(migration is not required).");
      return outcome::success();
    }
    auto current = block_tree.getLastFinalized().hash;

    auto migrate_tree =
        [&storage,
         &trie_storage](common::Hash256 state_root) -> outcome::result<void> {
      auto batch = storage.createBatch();
      OUTCOME_TRY(trie_batch, trie_storage.getEphemeralBatchAt(state_root));
      auto cursor = trie_batch->trieCursor();
      while (cursor->isValid()) {
        auto value_and_hash = cursor->value_and_hash();
        if (!value_and_hash) {
          continue;
        }
        auto &[value, hash] = *value_and_hash;
        OUTCOME_TRY(present,
                    storage.getSpace(Space::kTrieValue)->contains(hash));
        if (!present) {
          OUTCOME_TRY(batch->put(Space::kTrieValue, hash, value));
          OUTCOME_TRY(batch->remove(Space::kTrieNode, hash));
        }
        OUTCOME_TRY(cursor->next());
      }
      OUTCOME_TRY(batch->commit());
      return outcome::success();
    };
    // TODO: an error other than just an absent block may occur
    for (auto header = block_tree.getBlockHeader(current); header.has_value();
         header = block_tree.getBlockHeader(current)) {
      SL_VERBOSE(logger, "Migrating block {}...", header.value().blockInfo());
      OUTCOME_TRY(migrate_tree(header.value().state_root));
      current = header.value().parent_hash;
    }

    std::unordered_set<primitives::BlockHash> pending;
    for (auto leaf_hash : block_tree.getLeaves()) {
      pending.insert(leaf_hash);
    }
    while (!pending.empty()) {
      auto current_it = pending.begin();
      primitives::BlockHash current = *current_it;
      pending.erase(current_it);
      auto header = block_tree.getBlockHeader(current);
      // TODO: an error other than just an absent block may occur
      if (!header) {
        continue;
      }
      // already processed finalized blocks above
      if (header.value().parent_hash != block_tree.getLastFinalized().hash) {
        pending.insert(header.value().parent_hash);
      }
      SL_VERBOSE(logger, "Migrating block {}...", header.value().blockInfo());

      OUTCOME_TRY(migrate_tree(header.value().state_root));
    }

    SL_INFO(logger, "Trie storate migration ended successfully");
    return outcome::success();
  }

  outcome::result<void> runMigrations(injector::KagomeNodeInjector &injector) {
    auto block_tree = injector.injectBlockTree();
    auto trie_storage = injector.injectTrieStorage();
    auto storage = injector.injectStorage();

    OUTCOME_TRY(separateTrieValues(*block_tree, *trie_storage, *storage));
    return outcome::success();
  }
}  // namespace kagome::storage::migrations
