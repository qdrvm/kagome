/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/peer/address_repository/inmem_address_repository.hpp"

#include "libp2p/peer/errors.hpp"

namespace libp2p::peer {

  outcome::result<void> InmemAddressRepository::addAddresses(
      const PeerId &p, gsl::span<const multi::Multiaddress> ma,
      AddressRepository::Milliseconds ttl) {
    auto it = db_.find(p);
    if (it == db_.end()) {
      // no map allocated for given peer p
      db_.insert({p, std::make_shared<ttlmap>()});
      it = db_.find(p);
    }

    for (auto &m : ma) {
      it->second->insert({m, Clock::now() + ttl});
      signal_added_(p, m);
    }

    return outcome::success();
  }

  outcome::result<void> InmemAddressRepository::upsertAddresses(
      const PeerId &p, gsl::span<const multi::Multiaddress> ma,
      AddressRepository::Milliseconds ttl) {
    auto it = db_.find(p);
    if (it == db_.end()) {
      // peer not found
      return addAddresses(p, ma, ttl);
    }

    auto expires_in = Clock::now() + ttl;
    for (const auto &m : ma) {
      (*it->second)[m] = expires_in;
    }

    return outcome::success();
  }

  outcome::result<std::list<multi::Multiaddress>>
  InmemAddressRepository::getAddresses(const PeerId &p) const {
    auto it = db_.find(p);
    if (it == db_.end()) {
      return PeerError::NotFound;
    }

    std::list<multi::Multiaddress> ma;
    for (auto &item : *it->second) {
      ma.push_back(item.first);
    }
    return ma;
  }

  void InmemAddressRepository::clear(const PeerId &p) {
    auto it = db_.find(p);
    if (it != db_.end()) {
      for (const auto &item : *it->second) {
        signal_removed_(p, item.first);
      }
      it->second->clear();
    }
  }

  void InmemAddressRepository::collectGarbage() {
    auto now = Clock::now();
    auto peer = db_.begin();
    auto peerend = db_.end();

    // for each peer
    while (peer != peerend) {
      auto &&maptr = peer->second;

      // remove all expired addresses
      auto ma = maptr->begin();
      auto maend = maptr->end();
      while (ma != maend) {
        if (now >= ma->second) {
          signal_removed_(peer->first, ma->first);
          // erase returns element next to deleted
          ma = maptr->erase(ma);
        } else {
          ++ma;
        }
      }

      // peer has no more addresses
      if (peer->second->empty()) {
        // erase returns element next to deleted
        peer = db_.erase(peer);
      } else {
        ++peer;
      }
    }
  }

}  // namespace libp2p::peer
