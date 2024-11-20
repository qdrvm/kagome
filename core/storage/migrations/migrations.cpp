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
    auto batch = trie_backend.batch();
    auto current = block_tree.getLastFinalized();
    // TODO: an error other than just an absent block may occur
    for (auto header = block_tree.getBlockHeader(current.hash);
         header.has_value();
         header = block_tree.getBlockHeader(current.hash)) {
      OUTCOME_TRY(batch,
                  trie_storage.getEphemeralBatchAt(header.value().state_root));
      auto cursor = batch->trieCursor();
      while (cursor->isValid()) {
        auto value = cursor->value_and_hash();
        if (!value) {
          continue;
        }
        if (value->hash) {
          auto &hash = *value->hash;
          batch->put(Space::kTrieValue, hash, )
        }
      }
    }
    return outcome::success();
  }

}  // namespace kagome::storage::migrations
