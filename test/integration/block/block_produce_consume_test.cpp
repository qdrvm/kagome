/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application_test_suite.hpp"
#include "authorship/impl/proposer_impl.hpp"
#include "blockchain/block_storage.hpp"
#include "common/hexutil.hpp"
#include "primitives/inherent_data.hpp"
#include "runtime/binaryen/runtime_api/tagged_transaction_queue_impl.hpp"
#include "scale/scale.hpp"
#include "testutil/outcome.hpp"
#include "testutil/runtime/common/basic_wasm_provider.hpp"

using namespace kagome::primitives;
using namespace kagome::common;
using namespace kagome::scale;
using namespace kagome;

class BlockProduceConsume : public ApplicationTestSuite {
 protected:
  void SetUp() override {
    getInjector().template create<std::shared_ptr<blockchain::BlockStorage>>();

    auto backend =
        getInjector()
            .template create<std::shared_ptr<storage::trie::TrieDbBackend>>();

    auto trie_db = storage::trie::PolkadotTrieDb::createEmpty(backend);

    auto configuration_storage =
        getInjector()
            .template create<
                std::shared_ptr<application::ConfigurationStorage>>();

    for (const auto &[key, val] : configuration_storage->getGenesis()) {
      if (auto res = trie_db->put(key, val); not res) {
        common::raise(res.error());
      }
    }
    _trieDb.reset(trie_db.release());
  }

  void TearDown() override {
    _txPool.reset();
    _proposer.reset();
    _trieDb.reset();
  }

  auto &getTxPool() {
    if (!_txPool) {
      _txPool =
          getInjector()
              .template create<
                  std::unique_ptr<transaction_pool::TransactionPoolImpl>>();
    }
    return _txPool;
  }

  auto &getProposer() {
    if (!_proposer) {
      _proposer =
          getInjector()
              .template create<std::unique_ptr<authorship::ProposerImpl>>();
    }
    return _proposer;
  }

  std::unique_ptr<transaction_pool::TransactionPool> _txPool;
  std::unique_ptr<authorship::Proposer> _proposer;
  std::shared_ptr<storage::trie::TrieDb> _trieDb;

  Buffer _initialState;
  Buffer _afterProduceState;
  Buffer _afterConsumeState;
  std::shared_ptr<crypto::Hasher> _hasher =
      std::make_shared<crypto::HasherImpl>();

  outcome::result<Block> produceBlock(std::vector<Extrinsic> extrinsics = {});
  outcome::result<void> consumeBlock(const Block &);
};

/**
 * @given some initial state
 * @when 1. produce empty block and get state 'A'
 * 	     2. rollback to initial state
 * 	     3. consume that same block and get state 'B'
 * @then states 'A' and 'B' must be equal
 */
TEST_F(BlockProduceConsume, EmptyBlock) {
  ASSERT_OUTCOME_SUCCESS(block, produceBlock());

  std::cout << "Pre seal block: " << Buffer(encode(block).value()).toHex()
            << std::endl;

  ASSERT_OUTCOME_SUCCESS_TRY(consumeBlock(block));

  ASSERT_EQ(_afterProduceState, _afterConsumeState);
}

/**
 * @given some initial state
 * @when 1. produce block contained transaction and get state 'A'
 * 	     2. rollback to initial state
 * 	     3. consume that same block and get state 'B'
 * @then states 'A' and 'B' must be equal
 */
TEST_F(BlockProduceConsume, NoEmptyBlock) {
  ASSERT_OUTCOME_SUCCESS(
      buffer,
      Buffer::fromHex(
          "2d0284ffd43593c715fdd31c61141abd04a99fd6822c8558854ccde39a5684e7a56d"
          "a27d01f40a68108bf61df0e9d0108ab8b621b354d233067514055fc77542aa84b647"
          "608335134d45c4b3040b8c2830217aa8350091774eaf3c22644d8e0c8db541438600"
          "00000600ff8eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794"
          "f26a48a10f"));

  ASSERT_OUTCOME_SUCCESS(extrinsic, decode<Extrinsic>(buffer));

  ASSERT_OUTCOME_SUCCESS(block, produceBlock({extrinsic}));

  std::cout << "Pre seal block: " << Buffer(encode(block).value()).toHex()
            << std::endl;

  ASSERT_OUTCOME_SUCCESS_TRY(consumeBlock(block));

  ASSERT_EQ(_afterProduceState, _afterConsumeState);
}

outcome::result<Block> BlockProduceConsume::produceBlock(
    std::vector<Extrinsic> extrinsics) {
  size_t current_slot = 0;

  // note: can to use block id by two ways: as number or as hash
  //  size_t best_block_id = 0;
  auto best_block_id =
      Hash256::fromHex(
          "b86008325a917cd553b122702d1346bf6f132db4ea17155a9eec733413dc9ecf")
          .value();

  size_t authority_index = 0;

  crypto::VRFOutput vrfOutput;
  auto output =
      Buffer::fromHex(
          "36f85a274626c4da486b5789a590b3f64ae1b465b14c8f663cdad3c26b375b7e")
          .value();
  auto proof =
      Buffer::fromHex(
          "febedfa945cdcdea5b1a65e568c55e4c51c084a43d5adf5d4e3d9e74104866012aa4"
          "ac091f1d90dfc7dddbd981e48560c02fe841db1619be34660a6e2538a003")
          .value();
  std::copy(output.begin(), output.end(), vrfOutput.output.begin());
  std::copy(proof.begin(), proof.end(), vrfOutput.proof.begin());

  _initialState = _trieDb->getRootHash();

  //	std::this_thread::sleep_for(std::chrono::seconds(5));

  if (!extrinsics.empty()) {
    auto runtime_manager = std::make_shared<runtime::binaryen::RuntimeManager>(
        std::make_shared<runtime::StorageWasmProvider>(_trieDb),
        std::make_shared<extensions::ExtensionFactoryImpl>(_trieDb),
        _hasher);

    auto api = std::make_shared<runtime::binaryen::TaggedTransactionQueueImpl>(
        runtime_manager);

    for (auto &extrinsic : extrinsics) {
      auto size = extrinsic.data.size();
      auto hash = _hasher->blake2b_256(extrinsic.data);

      OUTCOME_TRY(validate_result, api->validate_transaction(extrinsic));

      assert(_trieDb->getRootHash() == _initialState);

      OUTCOME_TRY(visit_in_place(
          validate_result,
          [&](const primitives::TransactionValidityError &e) {
            return visit_in_place(
                e,
                // return either invalid or unknown validity error
                [](const auto &validity_error) -> outcome::result<void> {
                  return validity_error;
                });
          },
          [&](const primitives::ValidTransaction &validity)
              -> outcome::result<void> {
            // compose Transaction object
            Transaction tx{extrinsic,
                           size,
                           hash,
                           validity.priority,
                           validity.longevity,
                           validity.requires,
                           validity.provides,
                           validity.propagate};

            // send to pool
            OUTCOME_TRY(getTxPool()->submitOne(std::move(tx)));

            return outcome::success();
          }));
    }
  }

  static const auto kTimestampId =
      InherentIdentifier::fromString("timstap0").value();
  static const auto kBabeSlotId =
      InherentIdentifier::fromString("babeslot").value();

  int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch())
                    .count();

  InherentData inherent_data;
  OUTCOME_TRY(inherent_data.putData<uint64_t>(kTimestampId, now));
  OUTCOME_TRY(inherent_data.putData(kBabeSlotId, current_slot));

  consensus::BabeBlockHeader babe_header{
      current_slot, vrfOutput, authority_index};
  OUTCOME_TRY(encoded_header, scale::encode(babe_header));

  auto babe_pre_digest = PreRuntime{{kBabeEngineId, Buffer{encoded_header}}};

  auto pre_seal_block_res =
      getProposer()->propose(best_block_id, inherent_data, {babe_pre_digest});

  _afterProduceState = _trieDb->getRootHash();

  return pre_seal_block_res;
}

outcome::result<void> BlockProduceConsume::consumeBlock(const Block &block) {
  auto backend =
      getInjector()
          .template create<std::shared_ptr<storage::trie::TrieDbBackend>>();

  auto trie_db = storage::trie::PolkadotTrieDb::createEmpty(backend);

  auto configuration_storage =
      getInjector()
          .template create<
              std::shared_ptr<application::ConfigurationStorage>>();

  for (const auto &[key, val] : configuration_storage->getGenesis()) {
    if (auto res = trie_db->put(key, val); not res) {
      common::raise(res.error());
    }
  }

  auto originalTrieDb =
      std::shared_ptr<storage::trie::TrieDb>(trie_db.release());

  assert(originalTrieDb->getRootHash() == _initialState);

  //	std::this_thread::sleep_for(std::chrono::seconds(5));

  auto runtime_manager = std::make_shared<runtime::binaryen::RuntimeManager>(
      std::make_shared<runtime::StorageWasmProvider>(originalTrieDb),
      std::make_shared<extensions::ExtensionFactoryImpl>(originalTrieDb),
      _hasher);

  auto core = std::make_unique<runtime::binaryen::CoreImpl>(runtime_manager);

  OUTCOME_TRY(core->execute_block(block));

  _afterConsumeState = _trieDb->getRootHash();

  return outcome::success();
}
