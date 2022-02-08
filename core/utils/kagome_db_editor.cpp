#include <boost/throw_exception.hpp>
#include <chrono>
#include <fstream>
#include <thread>

#include <backward.hpp>
#undef TRUE
#undef FALSE
#include <boost/di.hpp>
#include <soralog/impl/configurator_from_yaml.hpp>

#include "blockchain/block_storage_error.hpp"
#include "blockchain/impl/block_tree_impl.hpp"
#include "blockchain/impl/key_value_block_header_repository.hpp"
#include "blockchain/impl/key_value_block_storage.hpp"
#include "blockchain/impl/storage_util.hpp"
#include "common/outcome_throw.hpp"
#include "crypto/hasher/hasher_impl.hpp"
#include "network/impl/extrinsic_observer_impl.hpp"
#include "runtime/common/runtime_upgrade_tracker_impl.hpp"
#include "storage/changes_trie/impl/storage_changes_tracker_impl.hpp"
#include "storage/leveldb/leveldb.hpp"
#include "storage/predefined_keys.hpp"
#include "storage/trie/impl/trie_storage_backend_impl.hpp"
#include "storage/trie/impl/trie_storage_impl.hpp"
#include "storage/trie/polkadot_trie/polkadot_trie_factory_impl.hpp"
#include "storage/trie/serialization/polkadot_codec.hpp"
#include "storage/trie/serialization/trie_serializer_impl.hpp"
#include "utils/profiler.hpp"

namespace di = boost::di;
using namespace std::chrono_literals;
using namespace kagome;
using namespace storage::trie;

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
# ----------------
)");
}

class Configurator : public soralog::ConfiguratorFromYAML {
 public:
  Configurator() : ConfiguratorFromYAML(embedded_config) {}
};

enum ArgNum : uint8_t { DB_PATH = 1, STATE_HASH, MODE };
enum Command : uint8_t { COMPACT, DUMP };

namespace kagome::runtime {
  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(
      Stream &s,
      const runtime::RuntimeUpgradeTrackerImpl::RuntimeUpgradeData &d) {
    return s << d.block << d.state;
  }

  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(
      Stream &s, runtime::RuntimeUpgradeTrackerImpl::RuntimeUpgradeData &d) {
    return s >> d.block >> d.state;
  }
}  // namespace kagome::runtime

void usage() {
  std::string help(R"(
Kagome DB Editor
Usage:
    kagome-db-editor <db-path> <root-state> <command>

    <db-path>     full or relative path to kagome database. It is usually path
                    polkadot/db inside base path set in kagome options.
    <root-state>  root state hash in 0x prefixed hex format. [Optional]
    <command>
         dump:    dumps the state from the DB to file hex_full_state.yaml in
                    format ready for use in polkadot-test.
         compact: compacts the kagome DB. Leaves only keys of the state passed
                    as an arguments. Removes all other keys. [Default]

Example:
    kagome-db-editor base-path/polkadot/db 0x1e22e dump
    kagome-db-editor base-path/polkadot/db
)");
  std::cout << help;
};

outcome::result<std::unique_ptr<PersistentTrieBatch>> persistent_batch(
    const std::unique_ptr<TrieStorageImpl> &trie, const RootHash &hash) {
  OUTCOME_TRY(batch, trie->getPersistentBatchAt(hash));
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
  return std::move(batch);
}

void child_storage_root_hashes(
    const std::unique_ptr<PersistentTrieBatch> &batch,
    std::set<RootHash> &hashes) {
  auto log = log::createLogger("main", "kagome-db-editor");

  const auto &child_prefix = storage::kChildStorageDefaultPrefix;
  auto cursor = batch->trieCursor();
  auto res = cursor->seekUpperBound(child_prefix);
  if (res.has_value()) {
    auto key = cursor->key();
    while (key.has_value() && key.value().size() >= child_prefix.size()
           && key.value().subbuffer(0, child_prefix.size()) == child_prefix) {
      if (auto value_res = batch->tryGet(key.value());
          value_res.has_value() && value_res.value().has_value()) {
        log->trace("Found child root hash {}",
                   value_res.value().value().toHex());
        hashes.insert(
            common::Hash256::fromSpan(value_res.value().value()).value());
      }
      res = cursor->next();
      key = cursor->key();
    }
  }
}

int main(int argc, char *argv[]) {
  backward::SignalHandling sh;

  Command cmd;
  if (argc == 2 or argc == 3
      or (argc == 4 and not std::strcmp(argv[MODE], "compact"))) {
    cmd = COMPACT;
  } else if (argc == 4 and not std::strcmp(argv[MODE], "dump")) {
    cmd = DUMP;
  } else {
    usage();
    return 0;
  }

  auto logging_system = std::make_shared<soralog::LoggingSystem>(
      std::make_shared<Configurator>());
  std::ignore = logging_system->configure();
  log::setLoggingSystem(logging_system);

  auto log = log::createLogger("main", "kagome-db-editor");

  auto prefix = common::Buffer{blockchain::prefix::TRIE_NODE};
  bool need_additional_compaction = false;
  {
    auto factory = std::make_shared<PolkadotTrieFactoryImpl>();

    std::shared_ptr<storage::LevelDB> storage;
    try {
      storage =
          storage::LevelDB::create(argv[DB_PATH], leveldb::Options()).value();
    } catch (std::system_error &e) {
      log->error("{}", e.what());
      usage();
      return 0;
    }

    auto injector = di::make_injector(
        di::bind<TrieSerializer>.template to([](const auto &injector) {
          return std::make_shared<TrieSerializerImpl>(
              injector.template create<sptr<PolkadotTrieFactory>>(),
              injector.template create<sptr<Codec>>(),
              injector.template create<sptr<TrieStorageBackend>>());
        }),
        di::bind<TrieStorageBackend>.template to(
            [&storage, &prefix](const auto &) {
              auto backend =
                  std::make_shared<TrieStorageBackendImpl>(storage, prefix);
              return backend;
            }),
        di::bind<storage::changes_trie::ChangesTracker>.template to<storage::changes_trie::StorageChangesTrackerImpl>(),
        di::bind<Codec>.template to<PolkadotCodec>(),
        di::bind<PolkadotTrieFactory>.to(factory),
        di::bind<crypto::Hasher>.template to<crypto::HasherImpl>(),
        di::bind<blockchain::BlockHeaderRepository>.template to<blockchain::KeyValueBlockHeaderRepository>(),
        di::bind<network::ExtrinsicObserver>.template to<network::ExtrinsicObserverImpl>());

    auto hasher = injector.template create<sptr<crypto::Hasher>>();

    auto block_storage =
        check(blockchain::KeyValueBlockStorage::create({}, storage, hasher))
            .value();

    auto block_tree_leaves = check(block_storage->getBlockTreeLeaves()).value();

    BOOST_ASSERT_MSG(not block_tree_leaves.empty(),
                     "Must be known or calculated at least one leaf");

    primitives::BlockHash last_finalized_block_hash;
    primitives::BlockHeader last_finalized_block_header;

    // Backward search of finalized block
    for (auto hash = block_tree_leaves.front();;) {
      auto header =
          check(check(block_storage->getBlockHeader(hash)).value()).value();
      if (header.number == 0) {
        last_finalized_block_hash = hash;
        last_finalized_block_header = header;
        break;
      }

      auto j_res = block_storage->getJustification(hash);
      if (j_res.has_value()) {
        last_finalized_block_hash = hash;
        last_finalized_block_header = header;
        break;
      }

      check(j_res).value();

      hash = header.parent_hash;
    }

    auto header = last_finalized_block_header;

    auto finalized_block_state_root = header.state_root;
    log->trace("Autodetected finalized block is #{}, state root is {:l}",
               header.number,
               finalized_block_state_root);

    // we place the only existing state hash at runtime look up key
    // it won't work for code substitute
    {
      std::vector<runtime::RuntimeUpgradeTrackerImpl::RuntimeUpgradeData>
          runtime_upgrade_data{};
      runtime_upgrade_data.emplace_back(
          primitives::BlockInfo{
              header.number,
              hasher->blake2b_256(check(scale::encode(header)).value())},
          header.state_root);
      auto encoded_res = check(scale::encode(runtime_upgrade_data));
      [[maybe_unused]] auto res =
          check(storage->put(storage::kRuntimeHashesLookupKey,
                             common::Buffer(encoded_res.value())));
    }

    RootHash hash;
    if (argc == 2) {
      auto block_number = header.number + 1;
      for (;;) {
        auto header_res = block_storage->getBlockHeader(block_number++);
        if (not header_res.has_value() || not header_res.value().has_value()) {
          break;
        }
        hash = header_res.value().value().state_root;
      }
      log->trace("Autodetected best block number is #{}, state root is 0x{}",
                 block_number - 1,
                 hash.toHex());
    } else {
      hash = check(RootHash::fromHexWithPrefix(argv[STATE_HASH])).value();
    }

    auto trie =
        TrieStorageImpl::createFromStorage(
            injector.template create<sptr<Codec>>(),
            injector.template create<sptr<TrieSerializer>>(),
            injector
                .template create<sptr<storage::changes_trie::ChangesTracker>>())
            .value();

    if (COMPACT == cmd) {
      auto batch = check(persistent_batch(trie, hash)).value();
      auto finalized_batch =
          check(persistent_batch(trie, finalized_block_state_root)).value();

      std::vector<std::unique_ptr<PersistentTrieBatch>> child_batches;
      {
        std::set<RootHash> child_root_hashes;
        child_storage_root_hashes(batch, child_root_hashes);
        child_storage_root_hashes(finalized_batch, child_root_hashes);
        for (const auto &child_root_hash : child_root_hashes) {
          auto child_batch_res = persistent_batch(trie, child_root_hash);
          if (child_batch_res.has_value()) {
            child_batches.emplace_back(std::move(child_batch_res.value()));
          } else {
            log->error("Child batch 0x{} not found in the storage",
                       child_root_hash.toHex());
          }
        }
      }

      auto db_cursor = storage->cursor();
      auto db_batch = storage->batch();
      auto res = check(db_cursor->seek(prefix));
      int count = 0;
      {
        TicToc t2("Process DB.", log);
        while (db_cursor->isValid() && db_cursor->key().has_value()
               && db_cursor->key().value()[0] == prefix[0]) {
          auto res2 = check(db_batch->remove(db_cursor->key().value()));
          count++;
          if (not(count % 10000000)) {
            log->trace("{} keys were processed at the db.", count);
            res2 = check(db_batch->commit());
            dynamic_cast<storage::LevelDB *>(storage.get())
                ->compact(prefix, check(db_cursor->key()).value());
            db_cursor = storage->cursor();
            db_batch = storage->batch();
            res = check(db_cursor->seek(prefix));
          }
          res2 = check(db_cursor->next());
        }
        std::ignore = check(db_batch->commit());
      }
      log->trace("{} keys were processed at the db.", ++count);

      {
        TicToc t3("Commit state.", log);
        auto res3 = check(finalized_batch->commit());
        log->trace("{}", res3.value().toHex());
        res3 = check(batch->commit());
        log->trace("{}", res3.value().toHex());
        for (const auto &child_batch : child_batches) {
          res3 = check(child_batch->commit());
          log->trace("{}", res3.value().toHex());
        }
      }

      {
        TicToc t4("Compaction 1.", log);
        dynamic_cast<storage::LevelDB *>(storage.get())
            ->compact(common::Buffer(), common::Buffer());
      }

      need_additional_compaction = true;
    } else if (DUMP == cmd) {
      auto batch = check(trie->getEphemeralBatchAt(hash)).value();
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
        auto cursor = batch->trieCursor();
        auto res = check(cursor->next());
        ofs << "values:\n";
        count = 0;
        while (cursor->key().has_value()) {
          ofs << "  - "
              << check(batch->get(check(cursor->key()).value())).value()
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
        check(storage::LevelDB::create(argv[1], leveldb::Options())).value();
    dynamic_cast<storage::LevelDB *>(storage.get())
        ->compact(common::Buffer(), common::Buffer());
  }
}
