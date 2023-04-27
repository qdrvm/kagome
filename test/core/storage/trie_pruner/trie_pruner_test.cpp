/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <iostream>
#include <random>

#include "crypto/hasher/hasher_impl.hpp"
#include "mock/core/application/app_configuration_mock.hpp"
#include "mock/core/blockchain/block_tree_mock.hpp"
#include "mock/core/storage/persistent_map_mock.hpp"
#include "mock/core/storage/spaced_storage_mock.hpp"
#include "mock/core/storage/trie/serialization/trie_serializer_mock.hpp"
#include "mock/core/storage/trie/trie_storage_backend_mock.hpp"
#include "mock/core/storage/write_batch_mock.hpp"
#include "storage/trie/polkadot_trie/polkadot_trie_factory_impl.hpp"
#include "storage/trie/polkadot_trie/polkadot_trie_impl.hpp"
#include "storage/trie/serialization/polkadot_codec.hpp"
#include "storage/trie/serialization/trie_serializer_impl.hpp"
#include "storage/trie_pruner/impl/trie_pruner_impl.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/prepare_loggers.hpp"
#include "testutil/storage/polkadot_trie_printer.hpp"

using namespace kagome::storage;
using namespace kagome::storage::trie_pruner;
using namespace kagome::common;
namespace primitives = kagome::primitives;
namespace crypto = kagome::crypto;
using kagome::primitives::BlockHash;
using kagome::primitives::BlockHeader;
using kagome::primitives::BlockInfo;
using kagome::primitives::BlockNumber;
using testing::_;
using testing::A;
using testing::An;
using testing::Invoke;
using testing::Return;
using testing::ReturnRef;

auto hash_from_str(std::string_view str) {
  Hash256 hash;
  std::copy_n(str.begin(), str.size(), hash.begin());
  return hash;
}

auto hash_from_header(const BlockHeader &header) {
  static crypto::HasherImpl hasher;
  return hasher.blake2b_256(scale::encode(header).value());
}

class CodecMock : public trie::Codec {
 public:
  MOCK_METHOD(outcome::result<Buffer>,
              encodeNode,
              (const trie::TrieNode &opaque_node,
               std::optional<trie::StateVersion> version,
               const ChildVisitor &child_visitor),
              (const, override));
  MOCK_METHOD(outcome::result<std::shared_ptr<trie::TrieNode>>,
              decodeNode,
              (gsl::span<const uint8_t> encoded_data),
              (const, override));
  MOCK_METHOD(Buffer, merkleValue, (const BufferView &buf), (const, override));
  MOCK_METHOD(outcome::result<Buffer>,
              merkleValue,
              (const trie::OpaqueTrieNode &opaque_node,
               std::optional<trie::StateVersion> version,
               const ChildVisitor &child_visitor),
              (const, override));
  MOCK_METHOD(bool, isMerkleHash, (const BufferView &buf), (const, override));
  MOCK_METHOD(Hash256, hash256, (const BufferView &buf), (const, override));
};

class PolkadotTrieMock final : public trie::PolkadotTrie {
 public:
  PolkadotTrieMock(NodePtr root) : root{root} {
    BOOST_ASSERT(this->root != nullptr);
  }

  outcome::result<bool> contains(const face::View<Buffer> &key) const override {
    throw std::runtime_error{"Not implemented"};
  }

  bool empty() const override {
    throw std::runtime_error{"Not implemented"};
  }

  outcome::result<face::OwnedOrView<Buffer>> get(
      const face::View<Buffer> &key) const override {
    throw std::runtime_error{"Not implemented"};
  }

  outcome::result<std::optional<BufferOrView>> tryGet(
      const face::View<Buffer> &key) const override {
    throw std::runtime_error{"Not implemented"};
  }

  outcome::result<void> put(const BufferView &key,
                            BufferOrView &&value) override {
    throw std::runtime_error{"Not implemented"};
  }

  outcome::result<void> remove(const BufferView &key) override {
    throw std::runtime_error{"Not implemented"};
  }

  outcome::result<std::tuple<bool, uint32_t>> clearPrefix(
      const BufferView &prefix,
      std::optional<uint64_t> limit,
      const OnDetachCallback &callback) override {
    throw std::runtime_error{"Not implemented"};
  }

  NodePtr getRoot() override {
    return root;
  }

  ConstNodePtr getRoot() const override {
    return root;
  }

  outcome::result<ConstNodePtr> retrieveChild(const trie::BranchNode &parent,
                                              uint8_t idx) const override {
    throw std::runtime_error{"Not implemented"};
  }

  outcome::result<NodePtr> retrieveChild(const trie::BranchNode &parent,
                                         uint8_t idx) override {
    throw std::runtime_error{"Not implemented"};
  }

  outcome::result<void> retrieveValue(
      trie::ValueAndHash &value) const override {
    throw std::runtime_error{"Not implemented"};
  }

  outcome::result<NodePtr> getNode(
      ConstNodePtr parent, const trie::NibblesView &key_nibbles) override {
    throw std::runtime_error{"Not implemented"};
  }

  outcome::result<ConstNodePtr> getNode(
      ConstNodePtr parent,
      const trie::NibblesView &key_nibbles) const override {
    throw std::runtime_error{"Not implemented"};
  }

  outcome::result<void> forNodeInPath(
      ConstNodePtr parent,
      const trie::NibblesView &path,
      const std::function<outcome::result<void>(
          const trie::BranchNode &, uint8_t, const trie::TrieNode &)> &callback)
      const override {
    throw std::runtime_error{"Not implemented"};
  }

  std::unique_ptr<trie::PolkadotTrieCursor> trieCursor() const override {
    throw std::runtime_error{"Not implemented"};
  }

  NodePtr root;
};

enum NodeType { NODE, DUMMY };

struct TrieNodeDesc {
  NodeType type;
  Buffer merkleValue;
  std::map<uint8_t, TrieNodeDesc> children;
};

class TriePrunerTest : public testing::Test {
 public:
  void SetUp() {
    testutil::prepareLoggers(soralog::Level::DEBUG);
    auto config_mock =
        std::make_shared<kagome::application::AppConfigurationMock>();
    ON_CALL(*config_mock, statePruningDepth()).WillByDefault(Return(16));

    trie_storage_mock.reset(new trie::TrieStorageBackendMock);
    persistent_storage_mock.reset(
        new testing::NiceMock<kagome::storage::SpacedStorageMock>);
    serializer_mock.reset(new trie::TrieSerializerMock);
    codec_mock.reset(new CodecMock);
    hasher = std::make_shared<crypto::HasherImpl>();

    pruner_space = std::make_shared<testing::NiceMock<BufferStorageMock>>();
    trie_pruner::TriePrunerImpl::TriePrunerInfo info{
        .prune_base = {0, "genesis"_hash256}, .child_states = {}};
    auto info_enc = scale::encode(info).value();
    static auto key = ":trie_pruner:info"_buf;
    ON_CALL(*pruner_space, tryGetMock(key.view()))
        .WillByDefault(
            Return(outcome::success(std::make_optional(Buffer{info_enc}))));
    ON_CALL(*pruner_space, put(key.view(), _))
        .WillByDefault(Return(outcome::success()));

    ON_CALL(*persistent_storage_mock, getSpace(kTriePruner))
        .WillByDefault(Invoke([this](auto) { return pruner_space; }));

    pruner.reset(TriePrunerImpl::create(config_mock,
                                        trie_storage_mock,
                                        serializer_mock,
                                        codec_mock,
                                        persistent_storage_mock,
                                        hasher)
                     .value()
                     .release());
  }

  void initOnBlock(BlockInfo base_block) {
    auto config_mock =
        std::make_shared<kagome::application::AppConfigurationMock>();
    ON_CALL(*config_mock, statePruningDepth()).WillByDefault(Return(16));
    trie_pruner::TriePrunerImpl::TriePrunerInfo info{.prune_base = base_block,
                                                     .child_states = {}};

    auto info_enc = scale::encode(info).value();
    static auto key = ":trie_pruner:info"_buf;
    ON_CALL(*pruner_space, tryGetMock(key.view()))
        .WillByDefault(
            Return(outcome::success(std::make_optional(Buffer{info_enc}))));

    pruner.reset(TriePrunerImpl::create(config_mock,
                                        trie_storage_mock,
                                        serializer_mock,
                                        codec_mock,
                                        persistent_storage_mock,
                                        hasher)
                     .value()
                     .release());
  }

  auto makeTrie(TrieNodeDesc desc) const {
    auto trie = std::make_shared<PolkadotTrieMock>(
        std::static_pointer_cast<trie::TrieNode>(makeNode(desc)));

    return trie;
  }

  std::shared_ptr<trie::OpaqueTrieNode> makeNode(TrieNodeDesc desc) const {
    if (desc.type == NODE) {
      if (desc.children.empty()) {
        auto node = std::make_shared<trie::LeafNode>(
            trie::KeyNibbles{}, trie::ValueAndHash{desc.merkleValue, {}});
        return node;
      }
      auto node = std::make_shared<trie::BranchNode>(trie::KeyNibbles{},
                                                     desc.merkleValue);
      for (auto [idx, child] : desc.children) {
        node->children[idx] = makeNode(child);
      }
      return node;
    }
    if (desc.type == DUMMY) {
      return std::make_shared<trie::DummyNode>(desc.merkleValue);
    }
    return nullptr;
  }

  std::shared_ptr<trie::TrieNode> makeTransparentNode(TrieNodeDesc desc) const {
    BOOST_ASSERT(desc.type != DUMMY);
    return std::static_pointer_cast<trie::TrieNode>(makeNode(desc));
  }

  std::unique_ptr<TriePrunerImpl> pruner;
  std::shared_ptr<trie::TrieSerializerMock> serializer_mock;
  std::shared_ptr<trie::TrieStorageBackendMock> trie_storage_mock;
  std::shared_ptr<testing::NiceMock<SpacedStorageMock>> persistent_storage_mock;
  std::shared_ptr<CodecMock> codec_mock;
  std::shared_ptr<crypto::Hasher> hasher;
  std::shared_ptr<testing::NiceMock<BufferStorageMock>> pruner_space;
};

struct NodeRetriever {
  template <typename F>
  outcome::result<trie::PolkadotTrie::NodePtr> operator()(
      const std::shared_ptr<trie::OpaqueTrieNode> &node, const F &) {
    if (auto dummy = std::dynamic_pointer_cast<const trie::DummyNode>(node);
        dummy != nullptr) {
      auto decoded = decoded_nodes.at(dummy->db_key);
      return decoded;
    }
    if (auto trie_node = std::dynamic_pointer_cast<trie::TrieNode>(node);
        trie_node != nullptr) {
      return trie_node;
    }
    return nullptr;
  }

  std::map<Buffer, std::shared_ptr<trie::TrieNode>> decoded_nodes;
};

auto setCodecExpectations(CodecMock &mock, trie::Codec &codec) {
  EXPECT_CALL(mock, encodeNode(_, _, _))
      .WillRepeatedly(Invoke([&codec](auto &node, auto ver, auto &visitor) {
        return codec.encodeNode(node, ver, visitor);
      }));
  EXPECT_CALL(mock, decodeNode(_)).WillRepeatedly(Invoke([&codec](auto n) {
    return codec.decodeNode(n);
  }));
  EXPECT_CALL(mock, merkleValue(_)).WillRepeatedly(Invoke([&codec](auto &v) {
    return codec.merkleValue(v);
  }));
  EXPECT_CALL(mock, merkleValue(_, _, _))
      .WillRepeatedly(Invoke([&codec](auto &node, auto ver, auto) {
        return codec.merkleValue(node, ver);
      }));
  EXPECT_CALL(mock, isMerkleHash(_)).WillRepeatedly(Invoke([&codec](auto &v) {
    return codec.isMerkleHash(v);
  }));
  EXPECT_CALL(mock, hash256(_)).WillRepeatedly(Invoke([&codec](auto &v) {
    return codec.hash256(v);
  }));
}

TEST_F(TriePrunerTest, BasicScenario) {
  auto codec = std::make_shared<trie::PolkadotCodec>();
  setCodecExpectations(*codec_mock, *codec);

  auto trie =
      makeTrie({NODE,
                "root1"_buf,
                {{0, {NODE, "_0"_buf, {}}}, {5, {NODE, "_5"_buf, {}}}}});
  ASSERT_OUTCOME_SUCCESS_TRY(
      pruner->addNewState(*trie, trie::StateVersion::V1));
  ASSERT_EQ(pruner->getTrackedNodesNum(), 3);

  auto trie_1 =
      makeTrie({NODE,
                "root2"_buf,
                {{0, {NODE, "_0"_buf, {}}}, {5, {NODE, "_5"_buf, {}}}}});
  ASSERT_OUTCOME_SUCCESS_TRY(
      pruner->addNewState(*trie_1, trie::StateVersion::V1));
  ASSERT_EQ(pruner->getTrackedNodesNum(), 4);

  EXPECT_CALL(
      *serializer_mock,
      retrieveNode(testing::A<const std::shared_ptr<trie::OpaqueTrieNode> &>(),
                   _))
      .WillRepeatedly(testing::Invoke(NodeRetriever{
          {{"_0"_buf, makeTransparentNode({NODE, "_0"_buf, {}})},
           {"_5"_buf, makeTransparentNode({NODE, "_5"_buf, {}})}}}));

  EXPECT_CALL(*trie_storage_mock, batch()).WillRepeatedly(Invoke([]() {
    auto batch = std::make_unique<face::WriteBatchMock<Buffer, Buffer>>();
    EXPECT_CALL(*batch, remove(_)).WillRepeatedly(Return(outcome::success()));
    EXPECT_CALL(*batch, commit()).WillOnce(Return(outcome::success()));
    return batch;
  }));
  EXPECT_CALL(*serializer_mock, retrieveTrie(BufferView("root1"_hash256), _))
      .WillOnce(testing::Return(trie));
  ASSERT_OUTCOME_SUCCESS_TRY(pruner->pruneFinalized(
      BlockHeader{.number = 1, .state_root = "root1"_hash256},
      kagome::primitives::BlockInfo{2, "block2"_hash256}));
  ASSERT_EQ(pruner->getTrackedNodesNum(), 3);

  EXPECT_CALL(*serializer_mock, retrieveTrie(BufferView{"root2"_hash256}, _))
      .WillOnce(testing::Return(trie_1));
  ASSERT_OUTCOME_SUCCESS_TRY(pruner->pruneFinalized(
      BlockHeader{.number = 2, .state_root = "root2"_hash256},
      kagome::primitives::BlockInfo{3, "block3"_hash256}));
  ASSERT_EQ(pruner->getTrackedNodesNum(), 0);
}

Buffer randomBuffer(std::mt19937 &rand) {
  Buffer buf;
  int size = rand() % 8;
  for (int i = 0; i < size; i++) {
    buf.putUint8(rand());
  }
  return buf;
}

template <typename F>
void forAllLoadedNodes(trie::TrieNode const &node, const F &f) {
  f(node);
  if (node.isBranch()) {
    auto &branch = static_cast<trie::BranchNode const &>(node);
    for (auto &child : branch.children) {
      if (auto transparent_child =
              dynamic_cast<const trie::TrieNode *>(child.get());
          transparent_child != nullptr) {
        forAllLoadedNodes(*transparent_child, f);
      }
    }
  }
}

template <typename F>
void forAllNodes(trie::PolkadotTrie &trie, trie::TrieNode &root, const F &f) {
  f(root);
  if (root.isBranch()) {
    auto &branch = static_cast<trie::BranchNode const &>(root);
    uint8_t idx = 0;
    for (auto &child : branch.children) {
      if (child != nullptr) {
        auto loaded_child = trie.retrieveChild(branch, idx).value();
        forAllNodes(trie, *loaded_child, f);
      }
      idx++;
    }
  }
}

std::set<Buffer> collectReferencedNodes(trie::PolkadotTrie &trie,
                                        trie::PolkadotCodec const &codec) {
  std::set<Buffer> res;
  if (trie.getRoot() == nullptr) {
    return {};
  }
  forAllNodes(trie, *trie.getRoot(), [&res, &codec](auto &node) {
    auto enc = codec.encodeNode(node, trie::StateVersion::V1, nullptr).value();
    res.insert(codec.merkleValue(enc));
  });
  return res;
}

void generateRandomTrie(size_t inserts,
                        trie::PolkadotTrie &trie,
                        std::set<Buffer> &inserted_keys) {
  std::mt19937 rand;
  rand.seed(42);

  for (unsigned j = 0; j < inserts; j++) {
    auto k = randomBuffer(rand);
    inserted_keys.insert(k);
    ASSERT_OUTCOME_SUCCESS_TRY(trie.put(k, randomBuffer(rand)));
  }
}

void makeRandomTrieChanges(size_t inserts,
                           size_t removes,
                           trie::PolkadotTrie &trie,
                           std::set<Buffer> &inserted_keys) {
  std::mt19937 rand;
  rand.seed(42);

  for (unsigned j = 0; j < inserts; j++) {
    auto k = randomBuffer(rand);
    inserted_keys.insert(k);
    ASSERT_OUTCOME_SUCCESS_TRY(trie.put(k, randomBuffer(rand)));
  }
  for (unsigned j = 0; j < removes; j++) {
    auto it = inserted_keys.begin();
    std::advance(it, rand() % inserted_keys.size());
    auto &k = *it;
    ASSERT_OUTCOME_SUCCESS_TRY(trie.remove(k));
    inserted_keys.erase(k);
  }
}

TEST_F(TriePrunerTest, RandomTree) {
  constexpr unsigned STATES_NUM = 30;
  constexpr unsigned INSERT_PER_STATE = 100;
  constexpr unsigned REMOVES_PER_STATE = 25;

  trie::PolkadotTrieImpl trie;
  auto codec = std::make_shared<trie::PolkadotCodec>();
  setCodecExpectations(*codec_mock, *codec);
  auto trie_factory = std::make_shared<trie::PolkadotTrieFactoryImpl>();

  std::map<Buffer, Buffer> node_storage;
  std::set<Buffer> inserted_keys;

  EXPECT_CALL(*trie_storage_mock, get(_))
      .WillRepeatedly(
          Invoke([&node_storage](auto &k) { return node_storage[k]; }));

  trie::TrieSerializerImpl serializer{trie_factory, codec, trie_storage_mock};
  std::vector<std::pair<Buffer, Buffer>> kv;
  std::mt19937 rand;
  rand.seed(42);

  std::vector<trie::RootHash> roots;

  std::set<Buffer> total_set;

  EXPECT_CALL(*serializer_mock, retrieveTrie(_, _))
      .WillRepeatedly(Invoke([&serializer](auto root, const auto &) {
        return serializer.retrieveTrie(root, nullptr);
      }));
  EXPECT_CALL(
      *serializer_mock,
      retrieveNode(A<const std::shared_ptr<trie::OpaqueTrieNode> &>(), _))
      .WillRepeatedly(Invoke([&serializer](auto &node, auto &) {
        return serializer.retrieveNode(node, nullptr);
      }));

  for (unsigned i = 0; i < STATES_NUM; i++) {
    EXPECT_CALL(*trie_storage_mock, batch()).WillOnce(Invoke([&node_storage]() {
      auto batch_mock =
          std::make_unique<face::WriteBatchMock<Buffer, Buffer>>();
      EXPECT_CALL(*batch_mock, put(_, _))
          .WillRepeatedly(Invoke([&node_storage](auto &k, auto &v) {
            node_storage[k] = v;
            return outcome::success();
          }));
      EXPECT_CALL(*batch_mock, commit())
          .WillRepeatedly(Return(outcome::success()));
      return batch_mock;
    }));

    for (unsigned j = 0; j < INSERT_PER_STATE; j++) {
      auto k = randomBuffer(rand);
      inserted_keys.insert(k);
      ASSERT_OUTCOME_SUCCESS_TRY(trie.put(k, randomBuffer(rand)));
    }
    for (unsigned j = 0; j < REMOVES_PER_STATE; j++) {
      auto it = inserted_keys.begin();
      std::advance(it, rand() % inserted_keys.size());
      auto &k = *it;
      ASSERT_OUTCOME_SUCCESS_TRY(trie.remove(k));
      inserted_keys.erase(k);
    }
    ASSERT_OUTCOME_SUCCESS_TRY(
        trie.clearPrefix(Buffer{{static_cast<uint8_t>(rand() % 256)}},
                         std::nullopt,
                         [](auto &, auto) -> outcome::result<void> {
                           return outcome::success();
                         }));
    auto new_set = collectReferencedNodes(trie, *codec);
    total_set.merge(new_set);
    ASSERT_OUTCOME_SUCCESS_TRY(
        pruner->addNewState(trie, trie::StateVersion::V0));
    std::set<Buffer> tracked_set;
    pruner->forRefCounts(
        [&](auto &node, auto count) { tracked_set.insert(node); });
    std::set<Buffer> diff;
    std::set_symmetric_difference(total_set.begin(),
                                  total_set.end(),
                                  tracked_set.begin(),
                                  tracked_set.end(),
                                  std::inserter(diff, diff.begin()));
    ASSERT_OUTCOME_SUCCESS(root,
                           serializer.storeTrie(trie, trie::StateVersion::V0));
    roots.push_back(root);

    if (i >= 16) {
      EXPECT_CALL(*trie_storage_mock, batch())
          .WillOnce(Invoke([&node_storage]() {
            auto batch =
                std::make_unique<face::WriteBatchMock<Buffer, Buffer>>();
            EXPECT_CALL(*batch, remove(_))
                .WillRepeatedly(Invoke([&node_storage](auto &k) {
                  node_storage.erase(k);
                  return outcome::success();
                }));
            EXPECT_CALL(*batch, commit()).WillOnce(Return(outcome::success()));
            return batch;
          }));

      auto &root = roots[i - 16];

      ASSERT_OUTCOME_SUCCESS_TRY(pruner->pruneFinalized(
          BlockHeader{.number = i - 16, .state_root = root},
          kagome::primitives::BlockInfo{i - 15, "some_hash"_hash256}));
    }
  }
  for (unsigned i = STATES_NUM - 16; i < STATES_NUM; i++) {
    EXPECT_CALL(*trie_storage_mock, batch()).WillOnce(Invoke([&node_storage]() {
      auto batch = std::make_unique<face::WriteBatchMock<Buffer, Buffer>>();
      EXPECT_CALL(*batch, remove(_))
          .WillRepeatedly(Invoke([&node_storage](auto &k) {
            node_storage.erase(k);
            return outcome::success();
          }));
      EXPECT_CALL(*batch, commit()).WillOnce(Return(outcome::success()));
      return batch;
    }));

    auto &root = roots[i];
    ASSERT_OUTCOME_SUCCESS_TRY(
        pruner->pruneFinalized(BlockHeader{.number = i, .state_root = root},
                               BlockInfo{i + 1, roots[i + 1]}));
  }
  for (auto &[hash, node] : node_storage) {
    std::cout << hash << "\n";
  }

  ASSERT_EQ(node_storage.size(), 0);
}

TEST_F(TriePrunerTest, RestoreStateFromGenesis) {
  auto block_tree = std::make_shared<kagome::blockchain::BlockTreeMock>();
  auto genesis_hash = "genesis"_hash256;
  ON_CALL(*block_tree, getGenesisBlockHash())
      .WillByDefault(ReturnRef(genesis_hash));

  std::map<BlockNumber, BlockHeader> headers;
  std::map<BlockHash, BlockNumber> hash_to_number;
  for (BlockNumber n = 1; n <= 6; n++) {
    auto parent_hash = headers.count(n - 1)
                         ? hash_from_header(headers.at(n - 1))
                         : "genesis"_hash256;
    headers[n] = BlockHeader{
        .parent_hash = parent_hash,
        .number = n,
        .state_root = hash_from_str("root_hash" + std::to_string(n)),
    };
    hash_to_number[hash_from_header(headers.at(n))] = n;
  }

  initOnBlock(BlockInfo{4, hash_from_header(headers.at(4))});

  ON_CALL(*block_tree, getBlockHash(_))
      .WillByDefault(Invoke([&headers](auto number) {
        return hash_from_header(headers.at(number));
      }));

  ON_CALL(*block_tree, getBlockHeader(_)).WillByDefault(Invoke([&](auto &hash) {
    if (hash == "genesis"_hash256) {
      return BlockHeader{.number = 0, .state_root = "genesis_root"_hash256};
    }
    return headers.at(hash_to_number.at(hash));
  }));

  ON_CALL(*block_tree, getChildren(_))
      .WillByDefault(Return(std::vector<kagome::primitives::BlockHash>{}));

  auto mock_block = [&](unsigned int number) {
    auto str_number = std::to_string(number);

    primitives::BlockHeader header = headers.at(number);
    auto root_hash = header.state_root;
    auto hash = hash_from_header(header);
    ON_CALL(*block_tree, getChildren(header.parent_hash))
        .WillByDefault(Return(std::vector{hash}));

    auto trie = std::make_shared<trie::PolkadotTrieImpl>();
    trie->put(Buffer::fromString("key" + str_number),
              Buffer::fromString("val" + str_number))
        .value();
    EXPECT_CALL(*serializer_mock, retrieveTrie(BufferView{root_hash}, _))
        .WillOnce(Return(trie));
    EXPECT_CALL(*codec_mock, merkleValue(testing::Ref(*trie->getRoot()), _, _))
        .WillRepeatedly(Return(Buffer::fromString("merkle_val" + str_number)));
    auto enc = Buffer::fromString("encoded_node" + str_number);
    EXPECT_CALL(*codec_mock, encodeNode(testing::Ref(*trie->getRoot()), _, _))
        .WillOnce(Return(enc));
    EXPECT_CALL(*codec_mock, hash256(testing::ElementsAreArray(enc)))
        .WillOnce(Return(root_hash));
  };
  mock_block(4);
  mock_block(5);
  mock_block(6);

  trie_pruner::TriePrunerImpl::TriePrunerInfo info{
      .prune_base = {4, hash_from_header(headers.at(4))}, .child_states = {}};
  auto info_enc = scale::encode(info).value();
  static auto key = ":trie_pruner:info"_buf;
  ON_CALL(*pruner_space, tryGetMock(key.view()))
      .WillByDefault(
          Return(outcome::success(std::make_optional(Buffer{info_enc}))));

  ON_CALL(*block_tree, getLastFinalized())
      .WillByDefault(Return(BlockInfo{4, hash_from_header(headers.at(4))}));
  ASSERT_OUTCOME_SUCCESS_TRY(pruner->init(*block_tree));
  ASSERT_EQ(pruner->getTrackedNodesNum(), 3);
}

std::unique_ptr<trie::PolkadotTrie> clone(const trie::PolkadotTrie &trie) {
  auto new_trie = std::make_unique<trie::PolkadotTrieImpl>();
  auto cursor = trie.trieCursor();
  EXPECT_OUTCOME_TRUE_1(cursor->next());
  while (cursor->isValid()) {
    EXPECT_OUTCOME_TRUE_1(
        new_trie->put(cursor->key().value(), cursor->value().value()));
    EXPECT_OUTCOME_TRUE_1(cursor->next());
  }
  return new_trie;
}

TEST_F(TriePrunerTest, FastSyncScenario) {
  auto block_tree = std::make_shared<kagome::blockchain::BlockTreeMock>();

  ON_CALL(*trie_storage_mock, batch()).WillByDefault(Invoke([]() {
    auto batch = std::make_unique<face::WriteBatchMock<Buffer, Buffer>>();
    ON_CALL(*batch, put(_, _)).WillByDefault(Return(outcome::success()));
    ON_CALL(*batch, commit()).WillByDefault(Return(outcome::success()));
    return batch;
  }));

  auto trie = std::make_shared<trie::PolkadotTrieImpl>();
  std::set<Buffer> inserted_keys;
  generateRandomTrie(100, *trie, inserted_keys);

  auto codec = std::make_shared<trie::PolkadotCodec>();
  setCodecExpectations(*codec_mock, *codec);

  auto trie_factory = std::make_shared<trie::PolkadotTrieFactoryImpl>();
  auto genesis_state_root = codec->hash256(
      codec->encodeNode(*trie->getRoot(), trie::StateVersion::V0).value());

  trie::TrieSerializerImpl serializer{trie_factory, codec, trie_storage_mock};

  ON_CALL(*serializer_mock, retrieveTrie(BufferView{genesis_state_root}, _))
      .WillByDefault(Return(trie));

  ON_CALL(*serializer_mock,
          retrieveNode(A<const std::shared_ptr<trie::OpaqueTrieNode> &>(), _))
      .WillByDefault(Invoke([&serializer](auto &node, auto &cb) {
        return serializer.retrieveNode(node, cb);
      }));

  BlockHeader genesis_header{
      .parent_hash = ""_hash256,
      .number = 0,
      .state_root = genesis_state_root,
  };
  ON_CALL(*block_tree, getBlockHeader(hash_from_header(genesis_header)))
      .WillByDefault(Return(genesis_header));

  ON_CALL(*block_tree, getGenesisBlockHash())
      .WillByDefault(
          testing::ReturnRefOfCopy(hash_from_header(genesis_header)));

  std::vector<BlockHeader> headers{genesis_header};
  std::vector<BlockHash> hashes{hash_from_header(genesis_header)};
  std::vector<std::shared_ptr<trie::PolkadotTrie>> tries{trie};
  for (BlockNumber n = 1; n < 100; n++) {
    std::shared_ptr<trie::PolkadotTrie> block_trie{clone(*tries[n - 1])};
    tries.push_back(block_trie);
    makeRandomTrieChanges(30, 10, *block_trie, inserted_keys);

    auto block_state_root = codec->hash256(
        codec->encodeNode(*trie->getRoot(), trie::StateVersion::V0).value());

    BlockHeader block_header{
        .parent_hash = hashes[n - 1],
        .number = n,
        .state_root = block_state_root,
    };
    auto hash = hash_from_header(block_header);
    ON_CALL(*block_tree, getBlockHash(n)).WillByDefault(Return(hash));
    ON_CALL(*block_tree, getBlockHeader(hash))
        .WillByDefault(Return(block_header));
    ON_CALL(*block_tree, getChildren(hashes[n - 1]))
        .WillByDefault(Return(std::vector{hash}));
    ON_CALL(*serializer_mock, retrieveTrie(BufferView{block_state_root}, _))
        .WillByDefault(Return(block_trie));

    headers.push_back(block_header);
    hashes.push_back(hash);
  }

  ASSERT_OUTCOME_SUCCESS_TRY(pruner->init(*block_tree));
}
