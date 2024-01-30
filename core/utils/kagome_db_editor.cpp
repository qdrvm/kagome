/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <boost/throw_exception.hpp>
#include <chrono>
#include <fstream>
#include <string_view>
#include <thread>

#if defined(BACKWARD_HAS_BACKTRACE)
#include <backward.hpp>
#endif

#undef TRUE
#undef FALSE

#include <boost/di.hpp>
#include <soralog/impl/configurator_from_yaml.hpp>

#include "blockchain/block_storage_error.hpp"
#include "blockchain/impl/block_header_repository_impl.hpp"
#include "blockchain/impl/block_storage_impl.hpp"
#include "blockchain/impl/block_tree_impl.hpp"
#include "blockchain/impl/storage_util.hpp"
#include "common/outcome_throw.hpp"
#include "crypto/blake2/blake2b.h"
#include "crypto/hasher/hasher_impl.hpp"
#include "network/impl/extrinsic_observer_impl.hpp"
#include "runtime/common/runtime_upgrade_tracker_impl.hpp"
#include "storage/predefined_keys.hpp"
#include "storage/rocksdb/rocksdb.hpp"
#include "storage/trie/impl/trie_storage_backend_impl.hpp"
#include "storage/trie/impl/trie_storage_impl.hpp"
#include "storage/trie/polkadot_trie/polkadot_trie_factory_impl.hpp"
#include "storage/trie/serialization/polkadot_codec.hpp"
#include "storage/trie/serialization/trie_serializer_impl.hpp"
#include "storage/trie_pruner/impl/trie_pruner_impl.hpp"
#include "utils/profiler.hpp"

namespace di = boost::di;
using namespace std::chrono_literals;
using namespace kagome;
using namespace storage::trie;
using common::BufferOrView;
using common::BufferView;

struct TrieTracker : TrieStorageBackend {
  TrieTracker(std::shared_ptr<TrieStorageBackend> inner)
      : inner{std::move(inner)} {}

  std::unique_ptr<Cursor> cursor() override {
    abort();
  }
  std::unique_ptr<storage::BufferBatch> batch() override {
    abort();
  }

  outcome::result<BufferOrView> get(const BufferView &key) const override {
    track(key);
    return inner->get(key);
  }
  outcome::result<std::optional<BufferOrView>> tryGet(
      const BufferView &key) const override {
    abort();
  }
  outcome::result<bool> contains(const BufferView &key) const override {
    abort();
  }
  bool empty() const override {
    abort();
  }

  outcome::result<void> put(const BufferView &key,
                            BufferOrView &&value) override {
    abort();
  }
  outcome::result<void> remove(const common::BufferView &key) override {
    abort();
  }

  void track(BufferView key) const {
    keys.emplace(common::Hash256::fromSpan(key).value());
  }
  bool tracked(BufferView key) const {
    return keys.count(common::Hash256::fromSpan(key).value());
  }

  std::shared_ptr<TrieStorageBackend> inner;
  mutable std::set<common::Hash256> keys;
};

template <class T>
using sptr = std::shared_ptr<T>;

template <typename T>
struct is_optional : std::false_type {};

template <typename T>
struct is_optional<typename std::optional<T>> : std::true_type {};

template <typename T>
inline auto check(T &&res) {
  if (not res.has_value()) {
    if constexpr (is_optional<T>::value) {
      throw std::runtime_error("No value");
    } else {
      kagome::common::raise(res.error());
    }
  }
  return std::forward<T>(res);
}

namespace {
  std::string embedded_config(R"(
# ----------------
sinks:
  - name: console
    type: console
    thread: none
    color: false
    latency: 0
groups:
  - name: main
    sink: console
    level: trace
    is_fallback: true
    children:
      - name: kagome-db-editor
      - name: trie
        level: debug
      - name: storage
      - name: changes_trie
      - name: blockchain
      - name: profile
# ----------------
)");
}

class Configurator : public soralog::ConfiguratorFromYAML {
 public:
  Configurator() : ConfiguratorFromYAML(embedded_config) {}
};

enum ArgNum : uint8_t { DB_PATH = 1, STATE_HASH, MODE };
enum Command : uint8_t { COMPACT, DUMP };

void usage() {
  std::string help(R"(
Kagome DB Editor - a storage pruner. Allows to reduce occupied disk space.
Usage:
    kagome db-editor <db-path>

    <db-path>     full or relative path to kagome database. It is usually path
                    polkadot/db inside base path set in kagome options.

Example:
    kagome-db-editor base-path/polkadot/db
)");
  std::cout << help;
};

outcome::result<std::unique_ptr<TrieBatch>> persistent_batch(
    const std::unique_ptr<TrieStorageImpl> &trie, const RootHash &hash) {
  OUTCOME_TRY(batch, trie->getPersistentBatchAt(hash, std::nullopt));
  auto cursor = batch->trieCursor();
  auto res = check(cursor->next());
  int count = 0;
  auto log = log::createLogger("main", "kagome-db-editor");
  {
    TicToc t1("Process state.", log);
    while (cursor->key().has_value()) {
      count++;
      res = check(cursor->next());
    }
  }
  log->trace("{} keys were processed at the state.", ++count);
  return batch;
}

void child_storage_root_hashes(const std::unique_ptr<TrieBatch> &batch,
                               std::set<RootHash> &hashes) {
  auto log = log::createLogger("main", "kagome-db-editor");

  const auto &child_prefix = storage::kChildStorageDefaultPrefix;
  auto cursor = batch->trieCursor();
  auto res = cursor->seekUpperBound(child_prefix);
  if (res.has_value()) {
    auto key = cursor->key();
    while (key.has_value() && startsWith(key.value(), child_prefix)) {
      if (auto value_res = batch->tryGet(key.value());
          value_res.has_value() && value_res.value().has_value()) {
        auto &value_opt = value_res.value();
        log->trace("Found child root hash {}", *value_opt);
        hashes.insert(common::Hash256::fromSpan(*value_opt).value());
      }
      res = cursor->next();
      key = cursor->key();
    }
  }
}

auto is_hash(const char *s) {
  return std::strlen(s) == common::Hash256::size() * 2 + 2
      && std::equal(s, s + 2, "0x");
};

int db_editor_main(int argc, const char **argv) {
#if defined(BACKWARD_HAS_BACKTRACE)
  backward::SignalHandling sh;
#endif

  Command cmd;
  if (argc == 2 or (argc == 3 && is_hash(argv[2]))
      or (argc == 4 and std::strcmp(argv[MODE], "compact") == 0)) {
    cmd = COMPACT;
  } else if (argc == 4 and std::strcmp(argv[MODE], "dump") == 0) {
    cmd = DUMP;
  } else {
    usage();
    return 0;
  }
  std::optional<RootHash> target_state_param;
  if (argc > 2) {
    if (!is_hash(argv[2])) {
      std::cout << "ERROR: Invalid state hash\n";
      usage();
      return -1;
    }
    target_state_param = RootHash::fromHexWithPrefix(argv[2]).value();
  }

  auto log = log::createLogger("main", "kagome-db-editor");

  common::Buffer prefix{};
  bool need_additional_compaction = false;
  {
    auto factory = std::make_shared<PolkadotTrieFactoryImpl>();

    std::shared_ptr<storage::RocksDb> storage;
    std::shared_ptr<storage::BufferStorage> buffer_storage;
    try {
      storage =
          storage::RocksDb::create(argv[DB_PATH], rocksdb::Options()).value();
      storage->dropColumn(storage::Space::kBlockBody);
      buffer_storage = storage->getSpace(storage::Space::kDefault);
    } catch (std::system_error &e) {
      log->error("{}", e.what());
      usage();
      return 0;
    }

    auto trie_node_tracker = std::make_shared<TrieTracker>(
        std::make_shared<TrieStorageBackendImpl>(storage));

    auto injector = di::make_injector(
        di::bind<TrieSerializer>.template to([](const auto &injector) {
          return std::make_shared<TrieSerializerImpl>(
              injector.template create<sptr<PolkadotTrieFactory>>(),
              injector.template create<sptr<Codec>>(),
              injector.template create<sptr<TrieStorageBackend>>());
        }),
        di::bind<TrieStorageBackend>.template to(trie_node_tracker),
        di::bind<storage::trie_pruner::TriePruner>.template to(
            std::shared_ptr<storage::trie_pruner::TriePruner>(nullptr)),
        di::bind<Codec>.template to([](const auto &injector) {
          return std::make_shared<PolkadotCodec>(kagome::crypto::blake2b<32>);
        }),
        di::bind<PolkadotTrieFactory>.to(factory),
        di::bind<crypto::Hasher>.template to<crypto::HasherImpl>(),
        di::bind<blockchain::BlockHeaderRepository>.template to<blockchain::BlockHeaderRepositoryImpl>(),
        di::bind<network::ExtrinsicObserver>.template to<network::ExtrinsicObserverImpl>());

    auto hasher = injector.template create<sptr<crypto::Hasher>>();

    auto block_storage =
        check(blockchain::BlockStorageImpl::create({}, storage, hasher))
            .value();

    auto block_tree_leaf_hashes =
        check(block_storage->getBlockTreeLeaves()).value();

    BOOST_ASSERT_MSG(not block_tree_leaf_hashes.empty(),
                     "Must be known or calculated at least one leaf");

    // Find the least and best leaf
    std::set<primitives::BlockInfo> leafs;
    primitives::BlockInfo least_leaf(
        std::numeric_limits<primitives::BlockNumber>::max(), {});
    primitives::BlockInfo best_leaf(
        std::numeric_limits<primitives::BlockNumber>::min(), {});
    for (auto hash : block_tree_leaf_hashes) {
      auto number = check(check(block_storage->getBlockHeader(hash)).value())
                        .value()
                        .number;
      const auto &leaf = *leafs.emplace(number, hash).first;
      SL_TRACE(log, "Leaf {} found", leaf);
      if (leaf.number <= least_leaf.number) {
        least_leaf = leaf;
      }
      if (leaf.number >= best_leaf.number) {
        best_leaf = leaf;
      }
    }

    primitives::BlockInfo last_finalized_block;
    primitives::BlockHeader last_finalized_block_header;
    storage::trie::RootHash last_finalized_block_state_root;
    storage::trie::RootHash after_finalized_block_state_root;

    std::set<primitives::BlockInfo> to_remove;

    // Backward search of finalized block and connect blocks to remove
    for (;;) {
      auto it = leafs.rbegin();
      auto node = leafs.extract((++it).base());
      auto &block = node.value();

      auto header =
          check(check(block_storage->getBlockHeader(block.hash)).value())
              .value();
      if (header.number == 0) {
        last_finalized_block = block;
        last_finalized_block_header = header;
        last_finalized_block_state_root = header.state_root;
        break;
      }

      auto justifications =
          check(block_storage->getJustification(block.hash)).value();
      if (justifications.has_value()) {
        last_finalized_block = block;
        last_finalized_block_header = header;
        last_finalized_block_state_root = header.state_root;
        break;
      }

      after_finalized_block_state_root = header.state_root;

      leafs.emplace(*header.parentInfo());
      to_remove.insert(std::move(node));
    }
    RootHash target_state =
        target_state_param.value_or(last_finalized_block_state_root);

    log->trace("Autodetected finalized block is {}, state root is {:l}",
               last_finalized_block,
               last_finalized_block_state_root);

    for (auto it = to_remove.rbegin(); it != to_remove.rend(); ++it) {
      check(block_storage->removeBlock(it->hash)).value();
    }

    SL_TRACE(log, "Save {} as single leaf", last_finalized_block);
    check(block_storage->setBlockTreeLeaves({last_finalized_block.hash}))
        .value();

    // we place the only existing state hash at runtime look up key
    // it won't work for code substitute
    {
      std::vector<runtime::RuntimeUpgradeTrackerImpl::RuntimeUpgradeData>
          runtime_upgrade_data{};
      runtime_upgrade_data.emplace_back(last_finalized_block,
                                        last_finalized_block_header.state_root);
      auto encoded_res = check(scale::encode(runtime_upgrade_data));
      check(buffer_storage->put(storage::kRuntimeHashesLookupKey,
                                common::Buffer(encoded_res.value())))
          .value();
    }

    auto trie =
        TrieStorageImpl::createFromStorage(
            injector.template create<sptr<Codec>>(),
            injector.template create<sptr<TrieSerializer>>(),
            injector.template create<sptr<storage::trie_pruner::TriePruner>>())
            .value();

    if (COMPACT == cmd) {
      auto batch = check(persistent_batch(trie, target_state)).value();
      auto finalized_batch =
          check(persistent_batch(trie, target_state)).value();

      std::vector<std::unique_ptr<TrieBatch>> child_batches;
      {
        std::set<RootHash> child_root_hashes;
        child_storage_root_hashes(batch, child_root_hashes);
        child_storage_root_hashes(finalized_batch, child_root_hashes);
        for (const auto &child_root_hash : child_root_hashes) {
          auto child_batch_res = persistent_batch(trie, child_root_hash);
          if (child_batch_res.has_value()) {
            child_batches.emplace_back(std::move(child_batch_res.value()));
          } else {
            log->error("Child batch {} not found in the storage",
                       child_root_hash);
          }
        }
      }

      auto trie_node_storage = storage->getSpace(storage::Space::kTrieNode);
      auto trie_value_storage = storage->getSpace(storage::Space::kTrieValue);

      auto track_trie_entries = [&log, &buffer_storage, &prefix](auto storage,
                                                                 auto tracker) {
        auto db_cursor = storage->cursor();
        auto db_batch = storage->batch();
        auto res = check(db_cursor->seekFirst());
        int count = 0;
        {
          TicToc t2("Process DB.", log);
          while (db_cursor->isValid() && db_cursor->key().has_value()) {
            auto key = db_cursor->key().value();
            if (tracker->tracked(key)) {
              db_cursor->next().value();
              continue;
            }
            auto res2 = check(db_batch->remove(key));
            count++;
            if (not(count % 10000000)) {
              log->trace("{} keys were processed at the db.", count);
              res2 = check(db_batch->commit());
              dynamic_cast<storage::RocksDbSpace *>(buffer_storage.get())
                  ->compact(prefix, check(db_cursor->key()).value());
              db_cursor = buffer_storage->cursor();
              db_batch = buffer_storage->batch();
              res = check(db_cursor->seek(key));
            }
            res2 = check(db_cursor->next());
          }
          std::ignore = check(db_batch->commit());
        }
        log->trace("{} keys were processed at the db.", ++count);
      };
      track_trie_entries(trie_node_storage, trie_node_tracker);

      {
        TicToc t4("Compaction 1.", log);
        dynamic_cast<storage::RocksDbSpace *>(buffer_storage.get())
            ->compact(common::Buffer(), common::Buffer());
      }

      need_additional_compaction = true;
    } else if (DUMP == cmd) {
      auto batch =
          check(trie->getEphemeralBatchAt(last_finalized_block.hash)).value();
      auto cursor = batch->trieCursor();
      auto res = check(cursor->next());
      {
        TicToc t1("Dump full state.", log);
        int count = 0;
        std::ofstream ofs;
        ofs.open("hex_full_state.yaml");
        ofs << "keys:\n";
        while (cursor->key().has_value()) {
          ofs << "  - " << cursor->key().value().toHex() << "\n";
          if (not(++count % 10000)) {
            log->trace("{} keys were dumped.", count);
          }
          res = cursor->next();
        }

        cursor = batch->trieCursor();
        res = check(cursor->next());
        ofs << "values:\n";
        count = 0;
        while (cursor->key().has_value()) {
          ofs << "  - "
              << check(batch->get(check(cursor->key()).value())).value().view()
              << "\n";
          if (not(++count % 50000)) {
            log->trace("{} values were dumped.", count);
          }
          res = check(cursor->next());
        }
        ofs.close();
      }
    }
  }

  if (need_additional_compaction) {
    TicToc t5("Compaction 2.", log);
    auto storage =
        check(storage::RocksDb::create(argv[1], rocksdb::Options())).value();
    auto buffer_storage = storage->getSpace(storage::Space::kDefault);
    dynamic_cast<storage::RocksDbSpace *>(buffer_storage.get())
        ->compact(common::Buffer(), common::Buffer());
  }

  return 0;
}
