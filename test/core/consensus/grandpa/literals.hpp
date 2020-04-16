/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef KAGOME_TEST_CORE_CONSENSUS_GRANDPA_LITERALS_HPP
#define KAGOME_TEST_CORE_CONSENSUS_GRANDPA_LITERALS_HPP

#include "consensus/grandpa/vote_weight.hpp"

inline kagome::consensus::grandpa::VoteWeight makeVoteWeight(uint64_t s) {
  kagome::consensus::grandpa::VoteWeight v;
  v.weight = s;
  return v;
}

inline kagome::consensus::grandpa::VoteWeight makeVoteWeight(std::string s) {
  size_t weight = 0;
  std::stringstream ss(s);
  ss >> weight;
  assert(!ss.fail());

  return makeVoteWeight(weight);
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

inline kagome::crypto::ED25519Signature makeSig(std::string s) {
  assert(s.size() <= kagome::crypto::ED25519Signature::size());
  kagome::crypto::ED25519Signature sig{};
  std::copy(s.begin(), s.end(), sig.begin());
  return sig;
}

inline kagome::consensus::grandpa::VoteWeight operator"" _W(const char *c,
                                                            size_t s) {
  return makeVoteWeight(std::string{c, c + s});
}

inline kagome::consensus::grandpa::BlockHash operator"" _H(const char *c,
                                                           size_t s) {
  return makeBlockHash(std::string{c, c + s});
}

inline kagome::consensus::grandpa::Id operator"" _ID(const char *c, size_t s) {
  return makeId(std::string{c, c + s});
}

inline kagome::crypto::ED25519Signature operator"" _SIG(const char *c,
                                                        size_t s) {
  return makeSig(std::string{c, c + s});
}

#endif  // KAGOME_TEST_CORE_CONSENSUS_GRANDPA_LITERALS_HPP
