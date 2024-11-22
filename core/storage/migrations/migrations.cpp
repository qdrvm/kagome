#include "storage/migrations/migrations.hpp"
#include <qtils/outcome.hpp>
#include <soralog/macro.hpp>

#include "blockchain/block_tree.hpp"
#include "blockchain/block_tree_error.hpp"
#include "injector/application_injector.hpp"
#include "log/logger.hpp"
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
    // TODO: an error other than just an absent block may occur
    for (auto header = block_tree.getBlockHeader(current); header.has_value();
         header = block_tree.getBlockHeader(current)) {
      SL_VERBOSE(logger, "Migrating block {}...", header.value().blockInfo());

      auto batch = storage.createBatch();
      OUTCOME_TRY(trie_batch,
                  trie_storage.getEphemeralBatchAt(header.value().state_root));
      auto cursor = trie_batch->trieCursor();
      while (cursor->isValid()) {
        auto value_and_hash = cursor->value_and_hash();
        if (!value_and_hash) {
          continue;
        }
        auto &[value, hash] = *value_and_hash;
        OUTCOME_TRY(batch->put(Space::kTrieValue, hash, value));
        OUTCOME_TRY(batch->remove(Space::kTrieNode, hash));
        OUTCOME_TRY(cursor->next());
      }
      OUTCOME_TRY(batch->commit());
      current = header.value().parent_hash;
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
