/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_KAD_XOR_DISTANCE_QUEUE_HPP
#define KAGOME_KAD_XOR_DISTANCE_QUEUE_HPP

#include <algorithm>

#include "crypto/sha/sha256.hpp"
#include "libp2p/protocol/kademlia/common.hpp"

namespace libp2p::protocol::kademlia {

  using kagome::common::Hash256;
  using kagome::crypto::sha256;

  inline auto xor_distance(const Hash256 &a, const Hash256 &b) {
    Hash256 x0r = a;
    // calculate XOR in-place
    for (size_t i = 0u, size = x0r.size(); i < size; ++i) {
      x0r[i] ^= b[i];
    }

    return x0r;
  }

  struct XorDistanceComp {
    explicit XorDistanceComp(const peer::PeerId &from) {
      hfrom = sha256(from.toVector());
    }

    bool operator()(const peer::PeerId &a, const peer::PeerId &b) {
      auto ha = sha256(a.toVector());
      auto hb = sha256(b.toVector());

      // dX is a xor distance from us (hfrom) to hX
      auto d1 = xor_distance(hfrom, ha);
      auto d2 = xor_distance(hfrom, hb);

      // return true, if distance d1 is less than d2, false otherwise
      return std::memcmp(d1.data(), d2.data(), d1.size()) < 0;
    }

    Hash256 hfrom;
  };

  class XorDistanceQueue {
   public:
    using iterator = std::vector<peer::PeerId>::iterator;

    explicit XorDistanceQueue(const peer::PeerId &from) : comp_(from) {}

    void push(peer::PeerId item) {
      heap_.push_back(std::move(item));
      std::push_heap(heap_.begin(), heap_.end(), comp_);
    }

    void pop() {
      std::pop_heap(heap_.begin(), heap_.end(), comp_);
      heap_.pop_back();
    }

    peer::PeerId &front() {
      return heap_.front();
    }

    const peer::PeerId &front() const {
      return heap_.front();
    }

    bool empty() const {
      return heap_.empty();
    }

   private:
    std::vector<peer::PeerId> heap_;
    XorDistanceComp comp_;
  };

}  // namespace libp2p::protocol::kademlia

#endif  // KAGOME_KAD_XOR_DISTANCE_QUEUE_HPP
