/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/peer/impl/peer_repository_impl.hpp"

#include "libp2p/multi/multiaddress.hpp"

namespace {

  template <typename T>
  inline void merge_sets(std::unordered_set<T> &out,
                         std::unordered_set<T> &&in) {
    out.insert(in.begin(), in.end());
  }

}  // namespace

namespace libp2p::peer {

  PeerRepositoryImpl::PeerRepositoryImpl(
      std::shared_ptr<AddressRepository> addrRepo,
      std::shared_ptr<KeyRepository> keyRepo,
      std::shared_ptr<ProtocolRepository> protocolRepo)
      : addr_(std::move(addrRepo)),
        key_(std::move(keyRepo)),
        proto_(std::move(protocolRepo)) {}

  AddressRepository &PeerRepositoryImpl::getAddressRepository() {
    return *addr_;
  }

  KeyRepository &PeerRepositoryImpl::getKeyRepository() {
    return *key_;
  }

  ProtocolRepository &PeerRepositoryImpl::getProtocolRepository() {
    return *proto_;
  }

  std::unordered_set<PeerId> PeerRepositoryImpl::getPeers() const {
    std::unordered_set<PeerId> peers;
    merge_sets<PeerId>(peers, addr_->getPeers());
    merge_sets<PeerId>(peers, key_->getPeers());
    merge_sets<PeerId>(peers, proto_->getPeers());
    return peers;
  }

  PeerInfo PeerRepositoryImpl::getPeerInfo(const PeerId &peer_id) const {
    auto peer_addrs_res = addr_->getAddresses(peer_id);
    if (!peer_addrs_res) {
      return PeerInfo{peer_id, {}};
    }
    return PeerInfo{peer_id, std::move(peer_addrs_res.value())};
  }

}  // namespace libp2p::peer
