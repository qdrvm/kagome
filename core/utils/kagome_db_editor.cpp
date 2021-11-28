#include <chrono>
#include <fstream>
#include <thread>

#include <backward.hpp>
#undef TRUE
#undef FALSE
#include <boost/di.hpp>
#include <soralog/impl/configurator_from_yaml.hpp>

#include "blockchain/impl/key_value_block_storage.hpp"
#include "blockchain/impl/storage_util.hpp"
#include "crypto/hasher/hasher_impl.hpp"
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
    <root-state>  root state hash in 0x prefixed hex format.
    <command>
         dump:    dumps the state from the DB to file hex_full_state.yaml in
                    format ready for use in polkadot-test.
         compact: compacts the kagome DB. Leaves only keys of the state passed
                    as an arguments. Removes all other keys. [Default]

Example:
    kagome-db-editor base-path/polkadot/db 0x1e22e dump
)");
  std::cout << help;
};

std::unique_ptr<PersistentTrieBatch> persistent_batch(
    const std::unique_ptr<TrieStorageImpl> &trie, const RootHash &hash) {
  auto batch = trie->getPersistentBatchAt(hash).value();
  auto cursor = batch->trieCursor();
  auto res = cursor->next();
  int count = 0;
  auto log = log::createLogger("main", "kagome-db-editor");
  {
    TicToc t1("Process state.", log);
    while (cursor->key().has_value()) {
      count++;
      res = cursor->next();
    }
  }
  log->trace("{} keys were processed at the state.", ++count);
  return batch;
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

    std::shared_ptr<storage::LevelDB> storage =
        storage::LevelDB::create(argv[DB_PATH], leveldb::Options()).value();
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
        di::bind<crypto::Hasher>.template to<crypto::HasherImpl>());

    RootHash finalized_hash;
    auto block_hash_res =
        storage->get(storage::kLastFinalizedBlockHashLookupKey);
    auto hasher = injector.template create<sptr<crypto::Hasher>>();
    auto block_storage_res = blockchain::KeyValueBlockStorage::loadExisting(
        storage, hasher, [](const auto &block) {});
    auto block_storage = block_storage_res.value();
    auto header =
        block_storage
            ->getBlockHeader(
                common::Hash256::fromSpan(block_hash_res.value()).value())
            .value();
    finalized_hash = header.state_root;
    log->trace("Autodetected finalized block is #{}, state root is 0x{}",
               header.number,
               finalized_hash.toHex());

    // we place the only existing state hash at runtime look up key
    // it won't work for code substitute
    {
      std::vector<runtime::RuntimeUpgradeTrackerImpl::RuntimeUpgradeData>
          runtime_upgrade_data{};
      runtime_upgrade_data.emplace_back(
          primitives::BlockInfo{
              header.number,
              hasher->blake2b_256(scale::encode(header).value())},
          header.state_root);
      auto encoded_res = scale::encode(runtime_upgrade_data);
      [[maybe_unused]] auto res =
          storage->put(storage::kRuntimeHashesLookupKey,
                       common::Buffer(encoded_res.value()));
    }

    RootHash hash;
    if (argc == 2) {
      auto block_number = header.number + 1;
      for (;;) {
        auto header_res = block_storage->getBlockHeader(block_number++);
        if (not header_res.has_value()) {
          break;
        }
        hash = header_res.value().state_root;
      }
      log->trace("Autodetected best block number is #{}, state root is 0x{}",
                 block_number - 1,
                 hash.toHex());
    } else {
      hash = RootHash::fromHexWithPrefix(argv[STATE_HASH]).value();
    }

    auto trie =
        TrieStorageImpl::createFromStorage(
            injector.template create<sptr<Codec>>(),
            injector.template create<sptr<TrieSerializer>>(),
            injector
                .template create<sptr<storage::changes_trie::ChangesTracker>>())
            .value();

    if (COMPACT == cmd) {
      auto batch = persistent_batch(trie, hash);
      auto finalized_batch = persistent_batch(trie, finalized_hash);

      auto db_cursor = storage->cursor();
      auto db_batch = storage->batch();
      auto res = db_cursor->seek(prefix);
      int count = 0;
      {
        TicToc t2("Process DB.", log);
        while (db_cursor->isValid() && db_cursor->key().has_value()
               && db_cursor->key().value()[0] == prefix[0]) {
          auto res2 = db_batch->remove(db_cursor->key().value());
          count++;
          if (not(count % 10000000)) {
            log->trace("{} keys were processed at the db.", count);
            res2 = db_batch->commit();
            dynamic_cast<storage::LevelDB *>(storage.get())
                ->compact(prefix, db_cursor->key().value());
            db_cursor = storage->cursor();
            db_batch = storage->batch();
            res = db_cursor->seek(prefix);
          }
          res2 = db_cursor->next();
        }
        std::ignore = db_batch->commit();
      }
      log->trace("{} keys were processed at the db.", ++count);

      {
        TicToc t3("Commit state.", log);
        auto res3 = finalized_batch->commit();
        log->trace("{}", res3.value().toHex());
        res3 = batch->commit();
        log->trace("{}", res3.value().toHex());
      }

      {
        TicToc t4("Compaction 1.", log);
        dynamic_cast<storage::LevelDB *>(storage.get())
            ->compact(common::Buffer(), common::Buffer());
      }

      need_additional_compaction = true;
    } else if (DUMP == cmd) {
      auto batch = trie->getEphemeralBatchAt(hash).value();
      auto cursor = batch->trieCursor();
      auto res = cursor->next();
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
        auto res = cursor->next();
        ofs << "values:\n";
        count = 0;
        while (cursor->key().has_value()) {
          ofs << "  - " << batch->get(cursor->key().value()).value() << "\n";
          if (not(++count % 50000)) {
            log->trace("{} values were dumped.", count);
          }
          res = cursor->next();
        }
        ofs.close();
      }
    }
  }

  if (need_additional_compaction) {
    TicToc t5("Compaction 2.", log);
    auto storage =
        storage::LevelDB::create(argv[1], leveldb::Options()).value();
    dynamic_cast<storage::LevelDB *>(storage.get())
        ->compact(common::Buffer(), common::Buffer());
  }
}
