#include <chrono>
#include <fstream>
#include <thread>

#include <boost/di.hpp>
#include <soralog/impl/configurator_from_yaml.hpp>

#include "blockchain/impl/storage_util.hpp"
#include "clock/profiler.hpp"
#include "storage/changes_trie/impl/storage_changes_tracker_impl.hpp"
#include "storage/leveldb/leveldb.hpp"
#include "storage/trie/impl/trie_storage_backend_impl.hpp"
#include "storage/trie/impl/trie_storage_impl.hpp"
#include "storage/trie/polkadot_trie/polkadot_trie_factory_impl.hpp"
#include "storage/trie/serialization/polkadot_codec.hpp"
#include "storage/trie/serialization/trie_serializer_impl.hpp"

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

int main(int argc, char *argv[]) {
  auto logging_system = std::make_shared<soralog::LoggingSystem>(
      std::make_shared<Configurator>());
  std::ignore = logging_system->configure();
  log::setLoggingSystem(logging_system);

  auto log = log::createLogger("main", "kagome-db-editor");

  auto prefix = common::Buffer{blockchain::prefix::TRIE_NODE};
  bool need_additional_compaction = false;
  {
    auto factory = std::make_shared<PolkadotTrieFactoryImpl>();

    auto storage =
        storage::LevelDB::create(argv[DB_PATH], leveldb::Options()).value();
    auto injector = di::make_injector(
        di::bind<TrieSerializer>.template to([](const auto &injector) {
          return std::make_shared<TrieSerializerImpl>(
              injector.template create<sptr<PolkadotTrieFactory>>(),
              injector.template create<sptr<Codec>>(),
              injector.template create<sptr<TrieStorageBackend>>());
        }),
        di::bind<TrieStorageBackend>.template to(
            [&storage, &prefix, &argv](const auto &) {
              auto backend =
                  std::make_shared<TrieStorageBackendImpl>(storage, prefix);
              return backend;
            }),
        di::bind<storage::changes_trie::ChangesTracker>.template to<storage::changes_trie::StorageChangesTrackerImpl>(),
        di::bind<Codec>.template to<PolkadotCodec>(),
        di::bind<PolkadotTrieFactory>.to(factory));

    auto hash = RootHash::fromHexWithPrefix(argv[STATE_HASH]).value();

    auto trie =
        TrieStorageImpl::createFromStorage(
            hash,
            injector.template create<sptr<Codec>>(),
            injector.template create<sptr<TrieSerializer>>(),
            injector
                .template create<sptr<storage::changes_trie::ChangesTracker>>())
            .value();

    if (argc == 3 or not std::strcmp(argv[MODE], "compact")) {
      auto batch = trie->getPersistentBatch().value();
      auto cursor = batch->trieCursor();
      auto res = cursor->next();
      int count = 0;
      {
        TicToc t1("Process state.", log);
        while (cursor->key().has_value()) {
          count++;
          res = cursor->next();
        }
      }
      log->trace("{} keys were processed at the state.", ++count);

      auto db_cursor = storage->cursor();
      auto db_batch = storage->batch();
      auto res2 = db_cursor->seek(prefix);
      count = 0;
      {
        TicToc t2("Process DB.", log);
        while (db_cursor->isValid() && db_cursor->key().has_value()
               && db_cursor->key().value()[0] == prefix[0]) {
          res = db_batch->remove(db_cursor->key().value());
          count++;
          if (not(count % 10000000)) {
            log->trace("{} keys were processed at the db.", count);
            res = db_batch->commit();
            dynamic_cast<storage::LevelDB *>(storage.get())
                ->compact(prefix, db_cursor->key().value());
            db_cursor = storage->cursor();
            db_batch = storage->batch();
            res2 = db_cursor->seek(prefix);
          }
          res = db_cursor->next();
        }
        res = db_batch->commit();
      }
      log->trace("{} keys were processed at the db.", ++count);

      {
        TicToc t3("Commit state.", log);
        auto res3 = batch->commit();
        log->trace("{}", res3.value().toHex());
      }

      {
        TicToc t4("Compaction 1.", log);
        dynamic_cast<storage::LevelDB *>(storage.get())
            ->compact(common::Buffer(), common::Buffer());
      }
      need_additional_compaction = true;
    } else if (not std::strcmp(argv[MODE], "dump")) {
      auto batch = trie->getEphemeralBatch().value();
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
