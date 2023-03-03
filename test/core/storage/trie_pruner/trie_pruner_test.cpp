/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <iostream>
#include <random>

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

using namespace kagome::storage;
using namespace kagome::storage::trie_pruner;
using namespace kagome::common;
using testing::_;
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
    storage.reset(new trie::TrieStorageBackendMock);
    serializer.reset(new trie::TrieSerializerMock);
    codec.reset(new CodecMock);
    pruner.reset(new TriePrunerImpl{storage, serializer, codec});
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
  std::shared_ptr<trie::TrieSerializerMock> serializer;
  std::shared_ptr<trie::TrieStorageBackendMock> storage;
  std::shared_ptr<CodecMock> codec;
};

struct NodeRetriever {
  outcome::result<trie::PolkadotTrie::NodePtr> operator()(
      const std::shared_ptr<trie::OpaqueTrieNode> &node) {
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
      *serializer,
      retrieveNode(testing::A<const std::shared_ptr<trie::OpaqueTrieNode> &>()))
      .WillRepeatedly(testing::Invoke(NodeRetriever{
          {{"_0"_buf, makeTransparentNode({NODE, "_0"_buf, {}})},
           {"_5"_buf, makeTransparentNode({NODE, "_5"_buf, {}})}}}));

  EXPECT_CALL(*serializer, retrieveTrie(Buffer{"root1"_hash256}))
      .WillOnce(testing::Return(trie));
  ASSERT_OUTCOME_SUCCESS_TRY(pruner->prune("root1"_hash256));
  ASSERT_EQ(pruner->getTrackedNodesNum(), 3);

  EXPECT_CALL(*serializer, retrieveTrie(Buffer{"root2"_hash256}))
      .WillOnce(testing::Return(trie_1));
  ASSERT_OUTCOME_SUCCESS_TRY(pruner->prune("root2"_hash256));
  ASSERT_EQ(pruner->getTrackedNodesNum(), 0);
}

TEST_F(TriePrunerTest, DummyNodes) {}

Buffer randomBuffer(std::mt19937 &rand) {
  Buffer buf;
  int size = rand() % 15;
  for (int i = 0; i < size; i++) {
    buf.putUint32(rand());
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
  trie::PolkadotTrieImpl trie;
  auto codec = std::make_shared<trie::PolkadotCodec>();
  setCodecExpectations(*this->codec, *codec);
  auto trie_factory = std::make_shared<trie::PolkadotTrieFactoryImpl>();
  auto backend = std::make_shared<trie::TrieStorageBackendMock>();

  std::map<Buffer, Buffer> node_storage;

  EXPECT_CALL(*backend, batch()).WillRepeatedly(Invoke([&node_storage]() {
    auto batch_mock = std::make_unique<face::WriteBatchMock<Buffer, Buffer>>();
    EXPECT_CALL(*batch_mock, put(_, _))
        .WillRepeatedly(Invoke([&node_storage](auto &k, auto &v) {
          node_storage[k] = v;
          return outcome::success();
        }));
    EXPECT_CALL(*batch_mock, commit())
        .WillRepeatedly(Return(outcome::success()));
    return batch_mock;
  }));
  EXPECT_CALL(*backend, get(_)).WillRepeatedly(Invoke([&node_storage](auto &k) {
    return node_storage[k];
  }));

  trie::TrieSerializerImpl serializer{trie_factory, codec, backend};
  std::vector<std::pair<Buffer, Buffer>> kv;
  std::mt19937 rand;
  rand.seed(42);
  for (int i = 0; i < 15; i++) {
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

  ASSERT_OUTCOME_SUCCESS_TRY(
      serializer.storeTrie(trie, trie::StateVersion::V1));
  auto original_set = collectReferencedNodes(trie);
  for (int i = 0; i < 10; i++) {
    for (int j = 0; j < 5; j++) {
      ASSERT_OUTCOME_SUCCESS_TRY(
          trie.put(randomBuffer(rand), randomBuffer(rand)));
    }
    size_t new_node_count = 0;
    forAllLoadedNodes(*trie.getRoot(),
                      [&new_node_count](auto &node) { new_node_count++; });
    auto old_ref_count = pruner->getTrackedNodesNum();
    ASSERT_OUTCOME_SUCCESS_TRY(pruner->addNewState(trie));
    ASSERT_EQ(pruner->getTrackedNodesNum(), old_ref_count + new_node_count);
    ASSERT_OUTCOME_SUCCESS_TRY(
        serializer.storeTrie(trie, trie::StateVersion::V1));
  }
}
