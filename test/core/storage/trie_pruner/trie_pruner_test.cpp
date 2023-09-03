/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <iostream>
#include <random>

#include "crypto/hasher/hasher_impl.hpp"
#include "mock/core/application/app_configuration_mock.hpp"
#include "mock/core/application/app_state_manager_mock.hpp"
#include "mock/core/blockchain/block_tree_mock.hpp"
#include "mock/core/storage/persistent_map_mock.hpp"
#include "mock/core/storage/spaced_storage_mock.hpp"
#include "mock/core/storage/trie/polkadot_trie_cursor_mock.h"
#include "mock/core/storage/trie/serialization/codec_mock.hpp"
#include "mock/core/storage/trie/serialization/trie_serializer_mock.hpp"
#include "mock/core/storage/trie/trie_storage_backend_mock.hpp"
#include "mock/core/storage/write_batch_mock.hpp"
#include "storage/database_error.hpp"
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
    auto cursor =
        std::make_unique<kagome::storage::trie::PolkadotTrieCursorMock>();
    EXPECT_CALL(*cursor, seekLowerBound(_))
        .WillRepeatedly(testing::Return(outcome::success()));
    return cursor;
  }
  NodePtr root;
};

enum NodeType { NODE, DUMMY };

struct TrieNodeDesc {
  NodeType type;
  Hash256 merkle_value;
  std::map<uint8_t, TrieNodeDesc> children;
};

class TriePrunerTest : public testing::Test {
 public:
  void SetUp() {
    testutil::prepareLoggers(soralog::Level::DEBUG);
    auto config_mock =
        std::make_shared<kagome::application::AppConfigurationMock>();
    ON_CALL(*config_mock, statePruningDepth()).WillByDefault(Return(16));

    trie_storage_mock.reset(
        new testing::NiceMock<trie::TrieStorageBackendMock>());
    persistent_storage_mock.reset(
        new testing::NiceMock<kagome::storage::SpacedStorageMock>);
    serializer_mock.reset(new testing::NiceMock<trie::TrieSerializerMock>);
    codec_mock.reset(new trie::CodecMock);
    hasher = std::make_shared<crypto::HasherImpl>();

    pruner_space = std::make_shared<testing::NiceMock<BufferStorageMock>>();
    trie_pruner::TriePrunerImpl::TriePrunerInfo info{.last_pruned_block = {}};
    auto info_enc = scale::encode(info).value();
    static auto key = ":trie_pruner:info"_buf;
    ON_CALL(*pruner_space, tryGetMock(key.view()))
        .WillByDefault(
            Return(outcome::success(std::make_optional(Buffer{info_enc}))));
    ON_CALL(*pruner_space, put(key.view(), _))
        .WillByDefault(Return(outcome::success()));

    ON_CALL(*persistent_storage_mock, getSpace(kDefault))
        .WillByDefault(Invoke([this](auto) { return pruner_space; }));

    pruner.reset(new TriePrunerImpl(
        std::make_shared<kagome::application::AppStateManagerMock>(),
        trie_storage_mock,
        serializer_mock,
        codec_mock,
        persistent_storage_mock,
        hasher,
        config_mock));
    ASSERT_TRUE(pruner->prepare());
  }

  void initOnLastPrunedBlock(BlockInfo last_pruned,
                             const kagome::blockchain::BlockTree &block_tree) {
    auto config_mock =
        std::make_shared<kagome::application::AppConfigurationMock>();
    ON_CALL(*config_mock, statePruningDepth()).WillByDefault(Return(16));
    trie_pruner::TriePrunerImpl::TriePrunerInfo info{.last_pruned_block =
                                                         last_pruned};

    auto info_enc = scale::encode(info).value();
    static auto key = ":trie_pruner:info"_buf;
    ON_CALL(*pruner_space, tryGetMock(key.view()))
        .WillByDefault(
            Return(outcome::success(std::make_optional(Buffer{info_enc}))));

    pruner.reset(new TriePrunerImpl(
        std::make_shared<kagome::application::AppStateManagerMock>(),
        trie_storage_mock,
        serializer_mock,
        codec_mock,
        persistent_storage_mock,
        hasher,
        config_mock));
    BOOST_ASSERT(pruner->prepare());
    ASSERT_OUTCOME_SUCCESS_TRY(pruner->recoverState(block_tree));
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
            trie::KeyNibbles{},
            trie::ValueAndHash{Buffer{desc.merkle_value}, {}});
        return node;
      }
      auto node = std::make_shared<trie::BranchNode>(trie::KeyNibbles{},
                                                     Buffer{desc.merkle_value});
      for (auto [idx, child] : desc.children) {
        node->children[idx] = makeNode(child);
      }
      return node;
    }
    if (desc.type == DUMMY) {
      return std::make_shared<trie::DummyNode>(desc.merkle_value);
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
  std::shared_ptr<trie::CodecMock> codec_mock;
  std::shared_ptr<crypto::Hasher> hasher;
  std::shared_ptr<testing::NiceMock<BufferStorageMock>> pruner_space;
};

struct NodeRetriever {
  template <typename F>
  outcome::result<trie::PolkadotTrie::NodePtr> operator()(
      const std::shared_ptr<trie::OpaqueTrieNode> &node, const F &) {
    if (auto dummy = std::dynamic_pointer_cast<const trie::DummyNode>(node);
        dummy != nullptr) {
      auto decoded = decoded_nodes.at(*dummy->db_key.asHash());
      return decoded;
    }
    if (auto trie_node = std::dynamic_pointer_cast<trie::TrieNode>(node);
        trie_node != nullptr) {
      return trie_node;
    }
    return nullptr;
  }

  std::map<Hash256, std::shared_ptr<trie::TrieNode>> decoded_nodes;
};

auto setCodecExpectations(trie::CodecMock &mock, trie::Codec &codec) {
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
  EXPECT_CALL(mock, hash256(_)).WillRepeatedly(Invoke([&codec](auto &v) {
    return codec.hash256(v);
  }));
  EXPECT_CALL(mock, shouldBeHashed(_, _))
      .WillRepeatedly(Invoke([&codec](auto &value, auto version) {
        return codec.shouldBeHashed(value, version);
      }));
}

#include "utils/struct_to_tuple.hpp"

struct TtP {
  int q;
  int m;
};

TEST_F(TriePrunerTest, NewScale) {
  kagome::scale::encode(TtP{10, 5}, TtP{7, 4});
}

TEST_F(TriePrunerTest, NewScaleVector) {
  std::vector<TtP> t = {{1,2}, {3,4}, {5,6}};
  kagome::scale::encode(t);
}
