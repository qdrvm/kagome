/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <algorithm>

#include "libp2p/peer/peer_info.hpp"

namespace libp2p::peer {
  PeerInfo::PeerInfo(const PeerInfo &peer_info) = default;
  PeerInfo &PeerInfo::operator=(const PeerInfo &peer_info) = default;
  PeerInfo::PeerInfo(PeerInfo &&peer_info) noexcept = default;
  PeerInfo &PeerInfo::operator=(PeerInfo &&peer_info) noexcept = default;

  PeerInfo::FactoryResult PeerInfo::createPeerInfo(const PeerId &peer_id) {
    if (peer_id.getType() != multi::HashType::sha256) {
      return FactoryError::kIdIsNotSha256Hash;
    }
    return PeerInfo{peer_id};
  }
  PeerInfo::FactoryResult PeerInfo::createPeerInfo(PeerId &&peer_id) {
    if (peer_id.getType() != multi::HashType::sha256) {
      return FactoryError::kIdIsNotSha256Hash;
    }
    return PeerInfo{std::move(peer_id)};
  }

  PeerInfo::PeerInfo(const PeerId &peer) : peer_id_{peer} {}
  PeerInfo::PeerInfo(PeerId &&peer) : peer_id_{std::move(peer)} {}

  PeerInfo &PeerInfo::addProtocols(
      gsl::span<multi::Multiaddress::Protocol> protocols) {
    protocols_.insert(protocols.cbegin(), protocols.cend());
    return *this;
  }

  bool PeerInfo::removeProtocol(multi::Multiaddress::Protocol protocol) {
    // erase returns number of elements removed
    return protocols_.erase(protocol) == 1;
  }

  PeerInfo &PeerInfo::addMultiaddresses(
      gsl::span<multi::Multiaddress> multiaddresses) {
    multiaddresses_.insert(multiaddresses.cbegin(), multiaddresses.cend());
    return *this;
  }

  PeerInfo &PeerInfo::addMultiaddresses(
      std::vector<multi::Multiaddress> &&multiaddresses) {
    multiaddresses_.insert(std::make_move_iterator(multiaddresses.begin()),
                           std::make_move_iterator(multiaddresses.end()));
    return *this;
  }

  bool PeerInfo::removeMultiaddress(const multi::Multiaddress &multiaddress) {
    // erase returns number of elements removed
    return multiaddresses_.erase(multiaddress) == 1;
  }

  bool PeerInfo::addMultiaddressSafe(const multi::Multiaddress &multiaddress) {
    auto address_it = std::find(observed_multiaddresses_.cbegin(),
                                observed_multiaddresses_.cend(), multiaddress);
    if (address_it != observed_multiaddresses_.cend()) {
      observed_multiaddresses_.erase(address_it);
      this->addMultiaddresses({multiaddress});
      return true;
    }
    observed_multiaddresses_.push_back(multiaddress);
    return false;
  }

  size_t PeerInfo::replaceMultiaddresses(
      gsl::span<multi::Multiaddress> to_remove,
      gsl::span<multi::Multiaddress> to_insert) {
    size_t not_removed_addresses = 0;
    std::for_each(
        to_remove.begin(), to_remove.end(),
        [this, &not_removed_addresses](const auto &to_remove_address) {
          // as erase uses binary search, complexity must be M*logN
          if (removeMultiaddress(to_remove_address)) {
            not_removed_addresses++;
          }
        });
    multiaddresses_.insert(to_insert.cbegin(), to_insert.cend());
    return not_removed_addresses;
  }
}  // namespace libp2p::peer
