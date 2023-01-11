/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_CORE_CONSENSUS_GRANDPA_VOTE_GRAPH_FIXTURE_HPP
#define KAGOME_TEST_CORE_CONSENSUS_GRANDPA_VOTE_GRAPH_FIXTURE_HPP

#include <gtest/gtest.h>

#include <rapidjson/document.h>

#include "consensus/grandpa/vote_graph/vote_graph_impl.hpp"
#include "consensus/grandpa/voter_set.hpp"
#include "core/consensus/grandpa/literals.hpp"
#include "mock/core/consensus/grandpa/chain_mock.hpp"
#include "testutil/outcome.hpp"

using namespace kagome;
using namespace consensus;
using namespace grandpa;

using testing::_;
using testing::Return;

struct VoteGraphFixture : public testing::Test {
  static const VoteType vt = VoteType::Prevote;

  const BlockHash GENESIS_HASH = "genesis"_H;

  std::shared_ptr<VoterSet> voter_set = [] {
    auto vs = std::make_shared<VoterSet>();

    std::ignore = vs->insert("w0_a"_ID, 0);

    std::ignore = vs->insert("w1_a"_ID, 1);
    std::ignore = vs->insert("w1_b"_ID, 1);
    std::ignore = vs->insert("w1_c"_ID, 1);

    std::ignore = vs->insert("w3_a"_ID, 3);
    std::ignore = vs->insert("w3_b"_ID, 3);
    std::ignore = vs->insert("w3_c"_ID, 3);

    std::ignore = vs->insert("w5_a"_ID, 5);
    std::ignore = vs->insert("w5_b"_ID, 5);
    std::ignore = vs->insert("w5_c"_ID, 5);

    std::ignore = vs->insert("w7_a"_ID, 7);
    std::ignore = vs->insert("w7_b"_ID, 7);
    std::ignore = vs->insert("w7_c"_ID, 7);

    std::ignore = vs->insert("w10_a"_ID, 10);
    std::ignore = vs->insert("w10_b"_ID, 10);
    std::ignore = vs->insert("w10_c"_ID, 10);

    return vs;
  }();

  std::shared_ptr<ChainMock> chain = std::make_shared<ChainMock>();
  std::shared_ptr<VoteGraphImpl> graph;

  template <typename... T>
  static std::vector<BlockHash> vec(T &&...t) {
    return std::vector<BlockHash>{t...};
  }

  void expect_getAncestry(const primitives::BlockHash &base,
                          const primitives::BlockHash &block,
                          const std::vector<BlockHash> &ancestry) {
    EXPECT_CALL(*chain, getAncestry(base, block)).WillOnce(Return(ancestry));
  }
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
  assert(document.HasMember("descendants"));
  assert(document.HasMember("cumulative_vote"));
  assert(document["number"].IsUint64());
  assert(document["ancestors"].IsArray());
  assert(document["descendants"].IsArray());
  assert(document["cumulative_vote"].IsUint64());

  e.number = document["number"].GetUint64();
  e.ancestors = jsonToHashArray(document["ancestors"]);
  e.descendants = jsonToHashArray(document["descendants"]);
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

  auto gh_ = graph.getHeads();
  auto jh_ = jsonToHeads(document);
  std::set<BlockHash> gh(gh_.begin(), gh_.end());
  std::set<BlockHash> jh(jh_.begin(), jh_.end());
  auto heads_is_equal = std::equal(
      gh.begin(), gh.end(), jh.begin(), jh.end(), [](auto &g, auto &j) {
        if (g != j) {
          std::cerr << "difference in hash" << std::endl;
          return false;
        }
        return true;
      });
  EXPECT_TRUE(heads_is_equal) << "head are incorrect";

  auto &ge_ = graph.getEntries();
  auto je_ = jsonToEntries(document);
  std::map<BlockHash, VoteGraph::Entry> ge(ge_.begin(), ge_.end());
  std::map<BlockHash, VoteGraph::Entry> je(je_.begin(), je_.end());
  auto is_equal = std::equal(
      ge.begin(), ge.end(), je.begin(), je.end(), [](auto &g, auto &j) {
        if (g.first != j.first) {
          std::cerr << "difference in hash" << std::endl;
          return false;
        }
        if (g.second.number != j.second.number) {
          std::cerr << "difference in number" << std::endl;
          return false;
        }
        if (g.second.ancestors != j.second.ancestors) {
          std::cerr << "difference in ancestors" << std::endl;
          return false;
        }
        if (g.second.descendants != j.second.descendants) {
          std::cerr << "difference in descendants" << std::endl;
          return false;
        }
        auto vt = VoteType::Prevote;
        if (g.second.cumulative_vote.sum(vt)
            != j.second.cumulative_vote.sum(vt)) {
          std::cerr << "difference in prevotes_sum" << std::endl;
          return false;
        }

        return g.first == j.first && g.second.number == j.second.number
               && g.second.ancestors == j.second.ancestors
               && g.second.descendants == j.second.descendants
               && g.second.cumulative_vote.sum(vt)
                      == j.second.cumulative_vote.sum(vt);
      });

  EXPECT_TRUE(is_equal) << "entries are incorrect";
}

#endif  // KAGOME_TEST_CORE_CONSENSUS_GRANDPA_VOTE_GRAPH_FIXTURE_HPP
