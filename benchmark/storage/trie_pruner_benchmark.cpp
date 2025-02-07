/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <benchmark/benchmark.h>
#include <rocksdb/options.h>

#include <boost/filesystem/operations.hpp>
#include <cstdlib>
#include <memory>
#include <random>
#include <soralog/level.hpp>
#include <soralog/macro.hpp>

#include "crypto/hasher/hasher_impl.hpp"
#include "gmock/gmock.h"
#include "log/logger.hpp"
#include "primitives/common.hpp"
#include "storage/in_memory/in_memory_spaced_storage.hpp"
#include "storage/rocksdb/rocksdb.hpp"
#include "storage/trie/impl/trie_storage_backend_impl.hpp"
#include "storage/trie/polkadot_trie/polkadot_trie_factory.hpp"
#include "storage/trie/polkadot_trie/polkadot_trie_factory_impl.hpp"
#include "storage/trie/serialization/polkadot_codec.hpp"
#include "storage/trie/serialization/trie_serializer_impl.hpp"
#include "storage/trie/types.hpp"
#include "storage/trie_pruner/impl/trie_pruner_impl.hpp"
#include "testutil/prepare_loggers.hpp"

#include "mock/core/application/app_configuration_mock.hpp"
#include "mock/core/application/app_state_manager_mock.hpp"
#include "utils/thread_pool.hpp"

namespace storage = kagome::storage;
namespace trie = storage::trie;

struct TriePrunerBenchmark {
  TriePrunerBenchmark() {
    testutil::prepareLoggers(soralog::Level::DEBUG);
    app_state_manager =
        std::make_shared<kagome::application::AppStateManagerMock>();
    EXPECT_CALL(*app_state_manager, atPrepare(testing::_))
        .WillRepeatedly(testing::Return());
    EXPECT_CALL(*app_state_manager, atLaunch(testing::_))
        .WillRepeatedly(testing::Return());
    EXPECT_CALL(*app_state_manager, atShutdown(testing::_))
        .WillRepeatedly(testing::Return());

    app_config = std::make_shared<kagome::application::AppConfigurationMock>();
    EXPECT_CALL(*app_config, statePruningDepth())
        .WillRepeatedly(testing::Return(100));
    EXPECT_CALL(*app_config, enableThoroughPruning())
        .WillRepeatedly(testing::Return(true));

    hasher = std::make_shared<kagome::crypto::HasherImpl>();
    codec = std::make_shared<trie::PolkadotCodec>();
    rocksdb::Options options{};
    options.create_if_missing = true;
    storage =
        storage::RocksDb::create(
            std::filesystem::path((boost::filesystem::temp_directory_path()
                                   / "kagome_pruner_benchmark"
                                   / boost::filesystem::unique_path())
                                      .string()),
            options)
            .value();
    storage_backend = std::make_shared<trie::TrieStorageBackendImpl>(storage);
    trie_factory = std::make_shared<trie::PolkadotTrieFactoryImpl>();
    serializer = std::make_shared<trie::TrieSerializerImpl>(
        trie_factory, codec, storage_backend);
    thread_pool = std::make_shared<kagome::common::WorkerThreadPool>(
        kagome::TestThreadPool{});
  }

  auto createPruner() {
    return std::make_unique<kagome::storage::trie_pruner::TriePrunerImpl>(
        app_state_manager,
        storage_backend,
        serializer,
        codec,
        storage,
        hasher,
        app_config,
        thread_pool);
  }

  std::shared_ptr<kagome::application::AppStateManagerMock> app_state_manager;
  std::shared_ptr<kagome::application::AppConfigurationMock> app_config;
  std::shared_ptr<kagome::crypto::HasherImpl> hasher;
  std::shared_ptr<trie::PolkadotCodec> codec;
  std::shared_ptr<storage::SpacedStorage> storage;
  std::shared_ptr<trie::TrieStorageBackendImpl> storage_backend;
  std::shared_ptr<trie::PolkadotTrieFactoryImpl> trie_factory;
  std::shared_ptr<trie::TrieSerializerImpl> serializer;
  std::shared_ptr<kagome::common::WorkerThreadPool> thread_pool;
};

auto createRandomTrie(trie::PolkadotTrieFactory &factory,
                      size_t values_num,
                      size_t max_value_len) {
  std::mt19937_64 random;

  auto trie = factory.createEmpty(trie::PolkadotTrie::RetrieveFunctions{});
  for (size_t i = 0; i < values_num; i++) {
    storage::Buffer key;
    key.resize(random() % 128);
    for (auto &byte : key) {
      byte = random() % 256;
    }
    storage::Buffer value;
    value.resize(random() % max_value_len);
    for (auto &byte : value) {
      byte = random() % 256;
    }
    trie->put(key, std::move(value)).value();
  }
  return trie;
}

static void registerStateBenchmark(benchmark::State &state) {
  TriePrunerBenchmark benchmark;

  auto trie = createRandomTrie(*benchmark.trie_factory, 10000, 70);
  auto logger = kagome::log::createLogger("Benchmark");

  for (auto _ : state) {
    auto pruner = benchmark.createPruner();
    pruner->addNewState(*trie, kagome::storage::trie::StateVersion::V1).value();
  }
}

static void pruneStateBenchmark(benchmark::State &state) {
  TriePrunerBenchmark benchmark;

  auto trie = createRandomTrie(*benchmark.trie_factory, 10000, 70);

  for (auto _ : state) {
    auto pruner = benchmark.createPruner();
    pruner->addNewState(*trie, trie::StateVersion::V1).value();
    auto root = benchmark.serializer
                    ->storeTrie(*trie, kagome::storage::trie::StateVersion::V1)
                    .value();
    pruner
        ->pruneFinalized(
            root,
            kagome::primitives::BlockInfo{kagome::primitives::BlockHash{}, 0})
        .value();
  }
}

BENCHMARK(registerStateBenchmark)
    ->Unit(benchmark::TimeUnit::kMillisecond)
    ->Iterations(10);

BENCHMARK(pruneStateBenchmark)
    ->Unit(benchmark::TimeUnit::kMillisecond)
    ->Iterations(10);

BENCHMARK_MAIN();
