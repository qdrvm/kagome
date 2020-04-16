/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application_test_suite.hpp"
#include "authorship/impl/proposer_impl.hpp"

using kagome::authorship::Proposer;
using kagome::authorship::ProposerImpl;
using kagome::common::Buffer;
using kagome::primitives::Block;
using kagome::primitives::Extrinsic;
using kagome::primitives::InherentData;
using kagome::primitives::PreRuntime;
using kagome::primitives::Transaction;

class BlockProduceConsume : public ApplicationTestSuite {
 protected:
  void SetUp() override {
    _trieDb =
        getInjector()
            .template create<std::shared_ptr<kagome::storage::trie::TrieDb>>();
    _now = std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
               .count();
  }

  void TearDown() override {
    _txPool.reset();
    _proposer.reset();
    _trieDb.reset();
  }

  auto &getTxPool() {
    if (!_txPool) {
      _txPool = getInjector()
                    .template create<std::unique_ptr<
                        kagome::transaction_pool::TransactionPoolImpl>>();
    }
    return _txPool;
  }

  auto &getProposer() {
    if (!_proposer) {
      _proposer =
          getInjector().template create<std::unique_ptr<ProposerImpl>>();
    }
    return _proposer;
  }

  std::unique_ptr<kagome::transaction_pool::TransactionPool> _txPool;
  std::unique_ptr<Proposer> _proposer;
  std::shared_ptr<kagome::storage::trie::TrieDb> _trieDb;

  int64_t _now;

  Buffer _initialState;
  Buffer _afterProduceState;
  Buffer _afterConsumeState;

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
  auto produce_result = produceBlock();
  ASSERT_TRUE(produce_result.has_value()) << produce_result.error().message();

  auto &block = produce_result.value();
  std::cout
      << "Pre seal block: "
      << kagome::common::Buffer(kagome::scale::encode(block).value()).toHex()
      << std::endl;

  auto consume_result = consumeBlock(block);
  ASSERT_TRUE(consume_result.has_value()) << consume_result.error().message();

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
  Extrinsic extrinsic{
      kagome::common::Buffer::fromHex(
          "290284ffdc3488acc1a6b90aa92cea0cfbe2b00754a74084970b08d968e948d4d3bf"
          "161a01c618a91e696bd798512750332e8c2487e3c66fed88f364ed8c40b911ca6e9e"
          "27edac6aa5f58f15703c82be84bb308754ac71d559c01a25c44ac8f9dabe14bb8800"
          "00000600ff488f6d1b0114674dcd81fd29642bc3bcec8c8366f6af0665860f9d4e8c"
          "8a972404")
          .value()};

  auto produce_result = produceBlock({extrinsic});
  ASSERT_TRUE(produce_result.has_value()) << produce_result.error().message();

  auto &block = produce_result.value();
  std::cout
      << "Pre seal block: "
      << kagome::common::Buffer(kagome::scale::encode(block).value()).toHex()
      << std::endl;

  auto consume_result = consumeBlock(block);
  ASSERT_TRUE(consume_result.has_value()) << consume_result.error().message();

  ASSERT_EQ(_afterProduceState, _afterConsumeState);
}

outcome::result<Block> BlockProduceConsume::produceBlock(
    std::vector<Extrinsic> extrinsics) {
  size_t current_slot = 0;

  // note: can to use block id by two ways: as number or as hash
  //	size_t best_block_id = 0;
  auto best_block_id =
      kagome::common::Hash256::fromHex(
          "b5ebfaf1fb6560d20e30a772c5482affeb5955602062a550b326b2e7135bb7a4")
          .value();

  size_t authority_index = 0;

  kagome::crypto::VRFOutput vrfOutput;
  auto output =
      kagome::common::Buffer::fromHex(
          "fa89e3354ef5b6438c57eff0358d237d81f03ac6af62840c3a4bf18ece2b214b")
          .value();
  auto proof =
      kagome::common::Buffer::fromHex(
          "ee2e8ad139e6a8036f36113e15730bc129316c5bc8f036ec3023488f6c74b30f0ee2"
          "aae8fb5e9dcc0ced913962b5284de25efeba750de145be68f75b9e5bea01")
          .value();
  std::copy(output.begin(), output.end(), vrfOutput.output.begin());
  std::copy(proof.begin(), proof.end(), vrfOutput.proof.begin());

  _initialState = _trieDb->getRootHash();

  if (!extrinsics.empty()) {
    for (auto &extrinsic : extrinsics) {
      auto size = extrinsic.data.size();
      auto hash = kagome::crypto::HasherImpl().blake2b_256(extrinsic.data);

      Transaction tx{std::move(extrinsic), size, hash, {}, {}, {}, {}, {}};

      OUTCOME_TRY(getTxPool()->submitOne(std::move(tx)));
    }
  }

  InherentData inherent_data;
  OUTCOME_TRY(
      inherent_data.putData<uint64_t>(kagome::consensus::kTimestampId, _now));
  OUTCOME_TRY(
      inherent_data.putData(kagome::consensus::kBabeSlotId, current_slot));

  kagome::consensus::BabeBlockHeader babe_header{
      current_slot, vrfOutput, authority_index};
  OUTCOME_TRY(encoded_header, kagome::scale::encode(babe_header));

  auto babe_pre_digest =
      PreRuntime{{kagome::primitives::kBabeEngineId, Buffer{encoded_header}}};

  auto pre_seal_block_res =
      getProposer()->propose(best_block_id, inherent_data, {babe_pre_digest});

  _afterProduceState = _trieDb->getRootHash();

  return pre_seal_block_res;
}

outcome::result<void> BlockProduceConsume::consumeBlock(const Block &block) {
  auto originalTrieDb = std::shared_ptr<kagome::storage::trie::TrieDb>(
      kagome::storage::trie::PolkadotTrieDb::createFromStorage(
          _initialState,
          getInjector()
              .template create<
                  std::shared_ptr<kagome::storage::trie::TrieDbBackend>>())
          .release());

  assert(originalTrieDb->getRootHash() == _initialState);

  auto core = std::make_unique<kagome::runtime::binaryen::CoreImpl>(
      std::make_shared<kagome::runtime::StorageWasmProvider>(originalTrieDb),
      std::make_shared<kagome::extensions::ExtensionFactoryImpl>(
          originalTrieDb));

  OUTCOME_TRY(core->execute_block(block));

  _afterConsumeState = _trieDb->getRootHash();

  return outcome::success();
}
