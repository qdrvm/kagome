#include "storage/migrations/migrations.hpp"

#include <iterator>
#include <queue>

#include <qtils/outcome.hpp>
#include <soralog/macro.hpp>

#include "blockchain/block_tree.hpp"
#include "blockchain/block_tree_error.hpp"
#include "common/blob.hpp"
#include "injector/application_injector.hpp"
#include "log/logger.hpp"
#include "primitives/common.hpp"
#include "storage/spaced_storage.hpp"
#include "storage/trie/trie_batches.hpp"
#include "storage/trie/trie_storage.hpp"
#include "storage/trie/trie_storage_backend.hpp"

namespace kagome::storage::migrations {

  outcome::result<void> migrateTree(SpacedStorage &storage,
                                    trie::TrieBatch &trie_batch,
                                    log::Logger &logger) {
    auto batch = storage.createBatch();
    auto cursor = trie_batch.trieCursor();
    OUTCOME_TRY(cursor->next());

    auto nodes = storage.getSpace(Space::kTrieNode);
    auto values = storage.getSpace(Space::kTrieValue);
    while (cursor->isValid()) {
      auto value_hash = cursor->valueHash();
      BOOST_ASSERT(value_hash.has_value());  // because cursor isValid
      if (value_hash->len == Hash256::size() + 1) {
        OUTCOME_TRY(present_in_values, values->contains(value_hash->hash));
        if (!present_in_values) {
          OUTCOME_TRY(value, nodes->get(value_hash->hash));
          OUTCOME_TRY(batch->put(
              Space::kTrieValue, value_hash->hash, std::move(value)));
          OUTCOME_TRY(batch->remove(Space::kTrieNode, value_hash->hash));
        }
      }

      OUTCOME_TRY(cursor->next());
    }
    OUTCOME_TRY(batch->commit());
    return outcome::success();
  }

  outcome::result<void> separateTrieValues(
      const blockchain::BlockTree &block_tree,
      const trie::TrieStorage &trie_storage,
      SpacedStorage &storage) {
    auto logger = log::createLogger("Migration", "storage");
    SL_INFO(logger,
            "Begin trie storage migration to separate nodes and values");
    if (storage.getSpace(Space::kTrieValue)->cursor()->isValid()) {
      SL_INFO(logger,
              "Stop trie storage migration, trie values column is not empty "
              "(migration is not required).");
      return outcome::success();
    }

    std::deque<primitives::BlockHash> pending;
    {
      OUTCOME_TRY(children,
                  block_tree.getChildren(block_tree.getLastFinalized().hash));
      std::ranges::copy(children, std::back_inserter(pending));
    }
    while (!pending.empty()) {
      primitives::BlockHash current = pending.front();
      pending.pop_front();
      auto header = block_tree.getBlockHeader(current);
      // TODO: an error other than just an absent block may occur
      if (!header) {
        continue;
      }
      OUTCOME_TRY(children, block_tree.getChildren(current));
      std::ranges::copy(children, std::back_inserter(pending));
      SL_VERBOSE(logger, "Migrating block {}...", header.value().blockInfo());
      OUTCOME_TRY(batch,
                  trie_storage.getEphemeralBatchAt(header.value().state_root));
      OUTCOME_TRY(migrateTree(storage, *batch, logger));
    }

    auto current = block_tree.getLastFinalized().hash;

    // TODO: an error other than just an absent block may occur
    for (auto header = block_tree.getBlockHeader(current); header.has_value();
         header = block_tree.getBlockHeader(current)) {
      SL_VERBOSE(logger, "Migrating block {}...", header.value().blockInfo());
      auto trie_batch_res =
          trie_storage.getEphemeralBatchAt(header.value().state_root);
      if (!trie_batch_res) {
        SL_VERBOSE(logger,
                   "State trie for block #{} is absent, assume we've reached "
                   "fast-synced blocks.",
                   header.value().number);
        break;
      }

      OUTCOME_TRY(migrateTree(storage, *trie_batch_res.value(), logger));
      current = header.value().parent_hash;
    }

    SL_INFO(logger, "Trie storage migration ended successfully");
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
