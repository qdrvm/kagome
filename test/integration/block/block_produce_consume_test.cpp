/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <testutil/runtime/common/basic_wasm_provider.hpp>
#include "application_test_suite.hpp"
#include "authorship/impl/proposer_impl.hpp"
#include "primitives/inherent_data.hpp"
#include "testutil/outcome.hpp"

using kagome::authorship::Proposer;
using kagome::authorship::ProposerImpl;
using kagome::common::Buffer;
using namespace kagome::primitives;

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
	ASSERT_OUTCOME_SUCCESS(block, produceBlock());

  std::cout
      << "Pre seal block: "
      << kagome::common::Buffer(kagome::scale::encode(block).value()).toHex()
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
  Extrinsic extrinsic{
      kagome::common::Buffer::fromHex(
          "290284ffdc3488acc1a6b90aa92cea0cfbe2b00754a74084970b08d968e948d4d3bf"
          "161a01c618a91e696bd798512750332e8c2487e3c66fed88f364ed8c40b911ca6e9e"
          "27edac6aa5f58f15703c82be84bb308754ac71d559c01a25c44ac8f9dabe14bb8800"
          "00000600ff488f6d1b0114674dcd81fd29642bc3bcec8c8366f6af0665860f9d4e8c"
          "8a972404")
          .value()};

	ASSERT_OUTCOME_SUCCESS(block, produceBlock({extrinsic}));

  std::cout
      << "Pre seal block: "
      << kagome::common::Buffer(kagome::scale::encode(block).value()).toHex()
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
    auto api =
        getInjector()
            .template create<
                std::shared_ptr<kagome::runtime::TaggedTransactionQueue>>();

    for (auto &extrinsic : extrinsics) {
      auto size = extrinsic.data.size();
      auto hash = kagome::crypto::HasherImpl().blake2b_256(extrinsic.data);

      OUTCOME_TRY(validity, api->validate_transaction(extrinsic));
      auto v = boost::get<kagome::primitives::ValidTransaction>(validity);

      Transaction tx{std::move(extrinsic),
                     size,
                     hash,
                     v.priority,
                     v.longevity,
                     v.requires,
                     v.provides,
                     v.propagate};

      OUTCOME_TRY(getTxPool()->submitOne(std::move(tx)));
    }
  }

	static const auto kTimestampId =
			InherentIdentifier::fromString("timstap0").value();
	static const auto kBabeSlotId =
			InherentIdentifier::fromString("babeslot").value();

	InherentData inherent_data;
  OUTCOME_TRY(
      inherent_data.putData<uint64_t>(kTimestampId, _now));
  OUTCOME_TRY(
      inherent_data.putData(kBabeSlotId, current_slot));

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

    auto extension_factory =
        std::make_shared<kagome::extensions::ExtensionFactoryImpl>(_trieDb);
    auto wasm_path = boost::filesystem::path(__FILE__).parent_path().string()
                     + "/wasm/polkadot_runtime.compact.wasm";
    auto wasm_provider =
        std::make_shared<kagome::runtime::BasicWasmProvider>(wasm_path);
    auto hasher = std::make_shared<kagome::crypto::HasherImpl>();

    auto runtime_manager =
        std::make_shared<kagome::runtime::binaryen::RuntimeManager>(
            std::move(wasm_provider),
            std::move(extension_factory),
            std::move(hasher));

  auto core = std::make_unique<kagome::runtime::binaryen::CoreImpl>(runtime_manager);

  OUTCOME_TRY(core->execute_block(block));

  _afterConsumeState = _trieDb->getRootHash();

  return outcome::success();
}
