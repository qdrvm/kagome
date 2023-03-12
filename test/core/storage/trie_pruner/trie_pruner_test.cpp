/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <iostream>
#include <random>

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
using kagome::primitives::BlockHeader;
using testing::_;
using testing::A;
using testing::An;
using testing::Invoke;
using testing::Return;

class CodecMock : public trie::Codec {
 public:
  MOCK_METHOD(outcome::result<Buffer>,
              encodeNode,
              (const trie::Node &opaque_node,
               trie::StateVersion version,
               const ChildVisitor &child_visitor),
              (const, override));
  MOCK_METHOD(outcome::result<std::shared_ptr<trie::Node>>,
              decodeNode,
              (gsl::span<const uint8_t> encoded_data),
              (const, override));
  MOCK_METHOD(Buffer, merkleValue, (const BufferView &buf), (const, override));
  MOCK_METHOD(bool, isMerkleHash, (const BufferView &buf), (const, override));
  MOCK_METHOD(Hash256, hash256, (const BufferView &buf), (const, override));
};

class CodecTestImpl final : public trie::Codec {
 public:
  ~CodecTestImpl() override = default;
  outcome::result<Buffer> encodeNode(
      const trie::Node &opaque_node,
      trie::StateVersion version,
      const ChildVisitor &child_visitor) const override {
    if (auto dummy = dynamic_cast<const trie::DummyNode *>(&opaque_node);
        dummy != nullptr) {
      return dummy->db_key;
    }
    if (auto node = dynamic_cast<const trie::TrieNode *>(&opaque_node);
        node != nullptr) {
      return node->getValue().value.value();
    }
    throw std::runtime_error{"Unknown node type"};
    return outcome::success();
  }

  outcome::result<std::shared_ptr<trie::Node>> decodeNode(
      gsl::span<const uint8_t> encoded_data) const override {
    throw std::runtime_error{"Not implemented"};
  }

  Buffer merkleValue(const BufferView &buf) const override {
    return buf;
  }

  bool isMerkleHash(const BufferView &buf) const override {
    return true;
  }

  Hash256 hash256(const BufferView &buf) const override {
    Hash256 hash;
    std::copy_n(
        buf.begin(), std::min(buf.size(), (long)Hash256::size()), hash.begin());
    return hash;
  }
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
    testutil::prepareLoggers();
    trie_storage_mock.reset(new trie::TrieStorageBackendMock);
    persistent_storage_mock.reset(new kagome::storage::SpacedStorageMock);
    serializer_mock.reset(new trie::TrieSerializerMock);
    codec_mock.reset(new CodecMock);
    pruner.reset(new TriePrunerImpl{trie_storage_mock,
                                    serializer_mock,
                                    codec_mock,
                                    persistent_storage_mock});
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
        node->setCachedHash(desc.merkleValue);
        return node;
      }
      auto node = std::make_shared<trie::BranchNode>(trie::KeyNibbles{},
                                                     desc.merkleValue);
      node->setCachedHash(desc.merkleValue);
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
  std::shared_ptr<SpacedStorageMock> persistent_storage_mock;
  std::shared_ptr<CodecMock> codec_mock;
};

struct NodeRetriever {
  template <typename F>
  outcome::result<trie::PolkadotTrie::NodePtr> operator()(
      const std::shared_ptr<trie::OpaqueTrieNode> &node, const F &) {
    if (auto dummy = std::dynamic_pointer_cast<const trie::DummyNode>(node);
        dummy != nullptr) {
      auto decoded = decoded_nodes.at(dummy->db_key);
      decoded->setCachedHash(dummy->db_key);
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

TEST_F(TriePrunerTest, BasicScenario) {
  EXPECT_CALL(*codec_mock, encodeNode(_, _, _))
      .WillRepeatedly(Invoke([](auto &node, auto, auto const &) {
        return static_cast<trie::OpaqueTrieNode const &>(node)
            .getCachedHash()
            .value();
      }));
  EXPECT_CALL(*codec_mock, merkleValue(_)).WillRepeatedly(Invoke([](auto &enc) {
    return enc;
  }));
  EXPECT_CALL(*codec_mock, hash256(_)).WillRepeatedly(Invoke([](auto &bytes) {
    Hash256 hash;
    std::copy_n(
        bytes.begin(), std::min(bytes.size(), (long)hash.size()), hash.begin());
    return hash;
  }));

  auto trie =
      makeTrie({NODE,
                "root1"_buf,
                {{0, {DUMMY, "_0"_buf, {}}}, {5, {DUMMY, "_5"_buf, {}}}}});
  ASSERT_OUTCOME_SUCCESS_TRY(pruner->addNewState(*trie));
  ASSERT_EQ(pruner->getTrackedNodesNum(), 3);

  auto trie_1 =
      makeTrie({NODE,
                "root2"_buf,
                {{0, {DUMMY, "_0"_buf, {}}}, {5, {DUMMY, "_5"_buf, {}}}}});
  ASSERT_OUTCOME_SUCCESS_TRY(pruner->addNewState(*trie_1));
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
  auto space_mock = std::make_shared<BufferStorageMock>();
  EXPECT_CALL(*space_mock, put(_, _)).WillOnce(Return(outcome::success()));
  EXPECT_CALL(*persistent_storage_mock, getSpace(_))
      .WillOnce(Return(space_mock));
  EXPECT_CALL(*serializer_mock, retrieveTrie(Buffer{"root1"_hash256}, _))
      .WillOnce(testing::Return(trie));
  ASSERT_OUTCOME_SUCCESS_TRY(
      pruner->prune(BlockHeader{.state_root = "root1"_hash256}));
  ASSERT_EQ(pruner->getTrackedNodesNum(), 3);

  EXPECT_CALL(*space_mock, put(_, _)).WillOnce(Return(outcome::success()));
  EXPECT_CALL(*persistent_storage_mock, getSpace(_))
      .WillOnce(Return(space_mock));
  EXPECT_CALL(*serializer_mock, retrieveTrie(Buffer{"root2"_hash256}, _))
      .WillOnce(testing::Return(trie_1));
  ASSERT_OUTCOME_SUCCESS_TRY(
      pruner->prune(BlockHeader{.state_root = "root2"_hash256}));
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
        auto child = trie.retrieveChild(branch, idx).value();
        forAllNodes(trie, *child, f);
      }
      idx++;
    }
  }
}

auto setCodecExpectations(CodecMock &mock, trie::Codec &codec) {
  EXPECT_CALL(mock, encodeNode(_, _, _))
      .WillRepeatedly(Invoke([&codec](auto &node, auto ver, auto &visitor) {
        return codec.encodeNode(node, ver, visitor);
      }));
  EXPECT_CALL(mock, decodeNode(_)).WillRepeatedly(Invoke([&codec](auto _) {
    return codec.decodeNode(_);
  }));
  EXPECT_CALL(mock, merkleValue(_)).WillRepeatedly(Invoke([&codec](auto &_) {
    return codec.merkleValue(_);
  }));
  EXPECT_CALL(mock, isMerkleHash(_)).WillRepeatedly(Invoke([&codec](auto &_) {
    return codec.isMerkleHash(_);
  }));
  EXPECT_CALL(mock, hash256(_)).WillRepeatedly(Invoke([&codec](auto &_) {
    return codec.hash256(_);
  }));
}

std::set<Buffer> collectReferencedNodes(trie::PolkadotTrie &trie,
                                        trie::PolkadotCodec const &codec) {
  std::set<Buffer> res;
  forAllNodes(trie, *trie.getRoot(), [&res, &codec](auto &node) {
    auto enc = codec.encodeNode(node, trie::StateVersion::V1, nullptr).value();
    res.insert(codec.merkleValue(enc));
  });
  return res;
}

TEST_F(TriePrunerTest, RandomTree) {
  constexpr unsigned STATES_NUM = 3;
  constexpr unsigned INSERT_PER_STATE = 5;

  trie::PolkadotTrieImpl trie;
  auto codec = std::make_shared<trie::PolkadotCodec>();
  setCodecExpectations(*codec_mock, *codec);
  auto trie_factory = std::make_shared<trie::PolkadotTrieFactoryImpl>();

  std::map<Buffer, Buffer> node_storage;

  EXPECT_CALL(*trie_storage_mock, batch())
      .WillRepeatedly(Invoke([&node_storage]() {
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
  EXPECT_CALL(*trie_storage_mock, get(_))
      .WillRepeatedly(
          Invoke([&node_storage](auto &k) { return node_storage[k]; }));

  trie::TrieSerializerImpl serializer{trie_factory, codec, trie_storage_mock};
  std::vector<std::pair<Buffer, Buffer>> kv;
  std::mt19937 rand;
  rand.seed(42);
  for (unsigned i = 0; i < INSERT_PER_STATE; i++) {
    kv.push_back({randomBuffer(rand), randomBuffer(rand)});
  }
  for (auto &[k, v] : kv) {
    ASSERT_OUTCOME_SUCCESS_TRY(trie.put(k, BufferView{v}));
  }
  ASSERT_OUTCOME_SUCCESS_TRY(pruner->addNewState(trie));

  size_t node_count = 0;
  forAllLoadedNodes(*trie.getRoot(),
                    [&node_count](auto &node) { node_count++; });
  ASSERT_EQ(pruner->getTrackedNodesNum(), node_count);

  ASSERT_OUTCOME_SUCCESS(root,
                         serializer.storeTrie(trie, trie::StateVersion::V1));

  std::vector<trie::RootHash> roots;
  roots.push_back(root);

  std::set<Buffer> total_set;
  auto current_set = collectReferencedNodes(trie, *codec);
  total_set.merge(current_set);
  for (unsigned i = 0; i < STATES_NUM; i++) {
    for (unsigned j = 0; j < INSERT_PER_STATE; j++) {
      ASSERT_OUTCOME_SUCCESS_TRY(
          trie.put(randomBuffer(rand), randomBuffer(rand)));
    }
    auto new_set = collectReferencedNodes(trie, *codec);
    total_set.merge(new_set);
    ASSERT_OUTCOME_SUCCESS_TRY(pruner->addNewState(trie));
    auto tracked_set = pruner->generateTrackedNodeSet();
    std::set<Buffer> diff;
    std::set_symmetric_difference(total_set.begin(),
                                  total_set.end(),
                                  tracked_set.begin(),
                                  tracked_set.end(),
                                  std::inserter(diff, diff.begin()));
    ASSERT_EQ(diff.size(), 0);
    ASSERT_OUTCOME_SUCCESS(root,
                           serializer.storeTrie(trie, trie::StateVersion::V1));
    roots.push_back(root);
    current_set = std::move(new_set);
  }
  EXPECT_CALL(*serializer_mock, retrieveTrie(_, _))
      .WillRepeatedly(Invoke([&serializer](auto &root, const auto &) {
        return serializer.retrieveTrie(root, nullptr);
      }));
  EXPECT_CALL(
      *serializer_mock,
      retrieveNode(A<const std::shared_ptr<trie::OpaqueTrieNode> &>(), _))
      .WillRepeatedly(Invoke([&serializer](auto &node, auto &) {
        return serializer.retrieveNode(node, nullptr);
      }));

  EXPECT_CALL(*trie_storage_mock, batch())
      .WillRepeatedly(Invoke([&node_storage]() {
        auto batch = std::make_unique<face::WriteBatchMock<Buffer, Buffer>>();
        EXPECT_CALL(*batch, remove(_))
            .WillRepeatedly(Invoke([&node_storage](auto &k) {
              node_storage.erase(k);
              return outcome::success();
            }));
        EXPECT_CALL(*batch, commit()).WillOnce(Return(outcome::success()));
        return batch;
      }));

  for (unsigned i = 0; i < STATES_NUM + 1; i++) {
    auto &root = roots[i];
    auto space_mock = std::make_shared<BufferStorageMock>();
    EXPECT_CALL(*space_mock, put(_, _)).WillOnce(Return(outcome::success()));
    EXPECT_CALL(*persistent_storage_mock, getSpace(_))
        .WillOnce(Return(space_mock));

    ASSERT_OUTCOME_SUCCESS_TRY(pruner->prune(BlockHeader{.state_root = root}));
  }
  for (auto &[hash, node] : node_storage) {
    std::cout << hash << "\n";
  }

  ASSERT_EQ(node_storage.size(), 0);
}
