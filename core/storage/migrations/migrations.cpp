#include "storage/migrations/migrations.hpp"
#include <qtils/outcome.hpp>

#include "blockchain/block_tree.hpp"
#include "blockchain/block_tree_error.hpp"
#include "storage/trie/trie_storage.hpp"
#include "storage/trie/trie_storage_backend.hpp"

namespace kagome::storage::migrations {

  outcome::result<void> separateTrieValues(
      const blockchain::BlockTree &block_tree,
      const trie::TrieStorage &trie_storage,
      trie::TrieStorageBackend &trie_backend) {
    auto current = block_tree.getLastFinalized();
    // TODO: an error other than just an absent block may occur
    for (auto header = block_tree.getBlockHeader(current.hash);
         header.has_value();
         header = block_tree.getBlockHeader(current.hash)) {
      auto batch = trie_backend.batch();
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
      }
      OUTCOME_TRY(batch->commit());
    }
    return outcome::success();
  }

}  // namespace kagome::storage::migrations
