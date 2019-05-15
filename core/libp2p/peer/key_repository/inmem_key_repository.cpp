/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/peer/key_repository/inmem_key_repository.hpp"

#include "libp2p/peer/errors.hpp"

namespace libp2p::peer {

  void InmemKeyRepository::clear(const PeerId &p) {
    auto it1 = pub_.find(p);
    if (it1 != pub_.end()) {
      // if vector is found, then clear it
      it1->second->clear();
    }

    auto it2 = kp_.find(p);
    if (it2 != kp_.end()) {
      // if vector is found, then clear it
      it2->second->clear();
    }
  }

  outcome::result<InmemKeyRepository::PubVecPtr>
  InmemKeyRepository::getPublicKeys(const PeerId &p) {
    auto it = pub_.find(p);
    if (it == pub_.end()) {
      return PeerError::NOT_FOUND;
    }

    return it->second;
  }

  void InmemKeyRepository::addPublicKey(const PeerId &p, const Pub &pub) {
    auto it = pub_.find(p);
    if (it != pub_.end()) {
      // vector if sound
      PubVecPtr &ptr = it->second;
      ptr->insert(pub);
      return;
    }

    // pub_[p] = [pub]
    auto ptr = std::make_shared<PubVec>();
    ptr->insert(pub);
    pub_.insert({p, std::move(ptr)});
  }

  outcome::result<InmemKeyRepository::KeyPairVecPtr>
  InmemKeyRepository::getKeyPairs(const PeerId &p) {
    auto it = kp_.find(p);
    if (it == kp_.end()) {
      return PeerError::NOT_FOUND;
    }

    return it->second;
  };

  void InmemKeyRepository::addKeyPair(const PeerId &p, const KeyPair &kp) {
    auto it = kp_.find(p);
    if (it != kp_.end()) {
      // vector if sound
      KeyPairVecPtr &ptr = it->second;
      ptr->insert(kp);
      return;
    }

    // kp_[p] = [kp]
    auto ptr = std::make_shared<KeyPairVec>();
    ptr->insert(kp);
    kp_.insert({p, std::move(ptr)});
  }

  std::unordered_set<PeerId> InmemKeyRepository::getPeers() const {
    std::unordered_set<PeerId> peers;

    for (const auto &it : kp_) {
      peers.insert(it.first);
    }

    for (const auto &it : pub_) {
      peers.insert(it.first);
    }

    return peers;
  };

}  // namespace libp2p::peer
