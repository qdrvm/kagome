/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CONSENSUS_GRANDPA_COMMON_HPP
#define KAGOME_CORE_CONSENSUS_GRANDPA_COMMON_HPP

#include <unordered_map>

#include "clock/clock.hpp"
#include "common/wrapper.hpp"
#include "crypto/ed25519_types.hpp"
#include "primitives/common.hpp"

namespace kagome::consensus::grandpa {

  using Id = primitives::AuthorityId;

  // vote signature
  using Signature = crypto::ED25519Signature;
  using BlockHash = primitives::BlockHash;
  using BlockNumber = primitives::BlockNumber;
  using RoundNumber = uint64_t;
  using MembershipCounter = uint64_t;

  struct VoterSet {
   public:
    VoterSet() : voters_{} {};
    VoterSet(std::vector<Id> authorities) : voters_{std::move(authorities)} {}

    void insert(Id voter, size_t weight) {
      voters_.push_back(voter);
      weight_map_.insert({voter, weight});
      total_weight_ += weight;
    }

    const std::vector<Id> &voters() const {
      return voters_;
    }

    size_t setId() const {
      return set_id_;
    }

    boost::optional<size_t> voterIndex(const Id &voter) const {
      auto it = std::find(voters_.begin(), voters_.end(), voter);
      if (it == voters_.end()) {
        return boost::none;
      }
      return std::distance(voters_.begin(), it);
    }

    boost::optional<size_t> voterWeight(const Id &voter) const {
      auto it = weight_map_.find(voter);
      if (it == weight_map_.end()) {
        return boost::none;
      }
      return it->second;
    }

    boost::optional<size_t> voterWeight(size_t voter_index) const {
      if (voter_index >= voters_.size()) {
        return boost::none;
      }
      return weight_map_.at(voters_[voter_index]);
    }

    size_t size() const {
      return voters_.size();
    }

    size_t totalWeight() const {
      return total_weight_;
    }

   private:
    std::vector<Id> voters_;
    size_t set_id_;
    std::unordered_map<Id, size_t> weight_map_;
    size_t total_weight_{0};
  };

  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const VoterSet &voters) {
    // TODO implement
    return s;
  }

  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, const VoterSet &voters) {
    // TODO implement
    return s >> voters;
  }

  using Clock = clock::SteadyClock;
  using Duration = Clock::Duration;
  using TimePoint = Clock::TimePoint;

  enum class State { START, PROPOSED, PREVOTED, PRECOMMITTED };
}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CORE_CONSENSUS_GRANDPA_COMMON_HPP
