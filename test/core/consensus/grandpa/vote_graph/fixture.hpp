/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_CORE_CONSENSUS_GRANDPA_VOTE_GRAPH_FIXTURE_HPP
#define KAGOME_TEST_CORE_CONSENSUS_GRANDPA_VOTE_GRAPH_FIXTURE_HPP

#include <gtest/gtest.h>

#include <rapidjson/document.h>

#include "consensus/grandpa/vote_graph/vote_graph_impl.hpp"
#include "core/consensus/grandpa/literals.hpp"
#include "mock/core/consensus/grandpa/chain_mock.hpp"
#include "testutil/outcome.hpp"

using namespace kagome;
using namespace consensus;
using namespace grandpa;

using ::testing::_;
using ::testing::Return;

struct VoteGraphFixture : public ::testing::Test {
  const BlockHash GENESIS_HASH = "genesis"_H;

  std::shared_ptr<ChainMock> chain = std::make_shared<ChainMock>();
  std::shared_ptr<VoteGraphImpl> graph;

  template <typename... T>
  static std::vector<BlockHash> vec(T &&... t) {
    return std::vector<BlockHash>{t...};
  }

  void expect_getAncestry(const primitives::BlockHash &base,
                          const primitives::BlockHash &block,
                          const std::vector<BlockHash> &ancestry) {
    EXPECT_CALL(*chain, getAncestry(base, block)).WillOnce(Return(ancestry));
  }

  const std::function<bool(const VoteWeight &, const VoteWeight &)> comparator =
      VoteWeight::prevoteComparator;
};

/// JSON utils to parse graph state

template <typename Json>
inline std::string jsonToString(Json &&json) {
  assert(json.IsString());
  return std::string{json.GetString(),
                     json.GetString() + json.GetStringLength()};
}

template <typename Array>
inline std::vector<BlockHash> jsonToHashArray(Array &&array) {
  assert(array.IsArray());
  std::vector<BlockHash> v{};
  v.reserve(array.Size());

  for (auto &item : array.GetArray()) {
    v.emplace_back(makeBlockHash(jsonToString(item)));
  }

  return v;
}

template <typename Object>
inline VoteGraph::Entry jsonToEntry(Object &&document) {
  VoteGraph::Entry e;

  assert(document.IsObject());
  assert(document.HasMember("number"));
  assert(document.HasMember("ancestors"));
  assert(document.HasMember("descendents"));
  assert(document.HasMember("cumulative_vote"));
  assert(document["number"].IsUint64());
  assert(document["ancestors"].IsArray());
  assert(document["descendents"].IsArray());
  assert(document["cumulative_vote"].IsUint64());

  e.number = document["number"].GetUint64();
  e.ancestors = jsonToHashArray(document["ancestors"]);
  e.descendents = jsonToHashArray(document["descendents"]);
  e.cumulative_vote = makeVoteWeight(document["cumulative_vote"].GetUint64());

  return e;
}

inline std::unordered_map<BlockHash, VoteGraph::Entry> jsonToEntries(
    rapidjson::Document &document) {
  std::unordered_map<BlockHash, VoteGraph::Entry> entries{};

  assert(document.IsObject());
  assert(document.HasMember("entries"));
  assert(document["entries"].IsObject());
  auto &&object = document["entries"].GetObject();
  for (auto &item : object) {
    BlockHash key = makeBlockHash(jsonToString(item.name));
    entries[key] = jsonToEntry(item.value);
  }

  return entries;
}

inline std::unordered_set<BlockHash> jsonToHeads(
    rapidjson::Document &document) {
  std::unordered_set<BlockHash> heads{};

  assert(document.IsObject());
  assert(document.HasMember("heads"));
  assert(document["heads"].IsArray());
  auto &&array = document["heads"].GetArray();
  for (auto &item : array) {
    heads.insert(makeBlockHash(jsonToString(item)));
  }

  return heads;
}

inline BlockInfo jsonToBlockInfo(rapidjson::Document &document) {
  assert(document.IsObject());
  assert(document.HasMember("base"));
  assert(document["base"].IsString());
  assert(document.HasMember("base_number"));
  assert(document["base_number"].IsUint64());
  BlockNumber number = document["base_number"].GetUint64();
  BlockHash hash = makeBlockHash(jsonToString(document["base"]));
  return BlockInfo(number, hash);
}

/**
 * Assert that \param graph and \param json are equal
 */
inline void AssertGraphCorrect(VoteGraphImpl &graph, std::string json) {
  rapidjson::Document document;
  document.Parse(json.c_str(), json.size());
  ASSERT_EQ(graph.getBase(), jsonToBlockInfo(document)) << "base is incorrect";
  ASSERT_EQ(graph.getHeads(), jsonToHeads(document)) << "heads are incorrect";
  EXPECT_EQ(graph.getEntries(), jsonToEntries(document))
      << "entries are incorrect";
}

/// Custom GTest Printers for custom types
namespace std {
  inline void PrintTo(const BlockHash &e, ostream *os) {
    *os << e;
  }

  inline void PrintTo(const BlockInfo &e, ostream *os) {
    *os << "BlockInfo{n=" << e.block_number << ", h=" << e.block_hash << "}";
  }

  inline void PrintTo(const VoteGraph::Entry &e, ostream *os) {
    *os << "Entry{";
    *os << "number=" << e.number;
    *os << ", ancestors=[";
    for (auto &a : e.ancestors) {
      *os << a << ", ";
    }
    *os << "], ";
    *os << "descendents=[";
    for (auto &a : e.descendents) {
      *os << a << ", ";
    }
    *os << "], ";
    *os << "cumulative_vote=" << e.cumulative_vote.prevotes_sum << "}";
  }
}  // namespace std
#endif  // KAGOME_TEST_CORE_CONSENSUS_GRANDPA_VOTE_GRAPH_FIXTURE_HPP
