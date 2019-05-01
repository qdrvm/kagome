/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/peer/peer_repository.hpp"

namespace {

  template <typename T>
  inline void merge_sets(std::unordered_set<T> &out,
                         std::unordered_set<T> &&in) {
    out.insert(in.begin(), in.end());
  }

}  // namespace

namespace libp2p::peer {

  PeerRepository::PeerRepository(
      std::shared_ptr<AddressRepository> addrRepo,
      std::shared_ptr<KeyRepository> keyRepo,
      std::shared_ptr<ProtocolRepository> protocolRepo)
      : addr_(std::move(addrRepo)),
        key_(std::move(keyRepo)),
        proto_(std::move(protocolRepo)) {}

  AddressRepository &PeerRepository::getAddressRepository() {
    return *addr_;
  }

  KeyRepository &PeerRepository::getKeyRepository() {
    return *key_;
  }

  ProtocolRepository &PeerRepository::getProtocolRepository() {
    return *proto_;
  }

  std::unordered_set<PeerId> PeerRepository::getPeers() const {
    std::unordered_set<PeerId> peers;
    merge_sets<PeerId>(peers, addr_->getPeers());
    merge_sets<PeerId>(peers, key_->getPeers());
    merge_sets<PeerId>(peers, proto_->getPeers());
    return peers;
  }

}  // namespace libp2p::peer
