/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_CORE_CONSENSUS_GRANDPA_LITERALS_HPP
#define KAGOME_TEST_CORE_CONSENSUS_GRANDPA_LITERALS_HPP

#include "consensus/grandpa/vote_weight.hpp"

inline kagome::consensus::grandpa::VoteWeight makeVoteWeight(uint64_t s) {
  kagome::consensus::grandpa::VoteWeight v;
  v.sum = s;
  return v;
}

inline kagome::consensus::grandpa::BlockHash makeBlockHash(std::string s) {
  assert(s.size() <= kagome::consensus::grandpa::BlockHash::size());
  kagome::consensus::grandpa::BlockHash hash{};
  std::copy(s.begin(), s.end(), hash.begin());
  return hash;
}

inline kagome::consensus::grandpa::Id makeId(std::string s) {
  assert(s.size() <= kagome::consensus::grandpa::Id::size());
  kagome::consensus::grandpa::Id id{};
  std::copy(s.begin(), s.end(), id.begin());
  return id;
}

inline kagome::crypto::Ed25519Signature makeSig(std::string s) {
  assert(s.size() <= kagome::crypto::Ed25519Signature::size());
  kagome::crypto::Ed25519Signature sig{};
  std::copy(s.begin(), s.end(), sig.begin());
  return sig;
}

inline kagome::consensus::grandpa::VoteWeight operator"" _W(
    unsigned long long weight) {
  return makeVoteWeight(weight);
}

inline kagome::consensus::grandpa::BlockHash operator"" _H(const char *c,
                                                           size_t s) {
  return makeBlockHash(std::string{c, c + s});
}

inline kagome::consensus::grandpa::Id operator"" _ID(const char *c, size_t s) {
  return makeId(std::string{c, c + s});
}

inline kagome::crypto::Ed25519Signature operator"" _SIG(const char *c,
                                                        size_t s) {
  return makeSig(std::string{c, c + s});
}

#endif  // KAGOME_TEST_CORE_CONSENSUS_GRANDPA_LITERALS_HPP
