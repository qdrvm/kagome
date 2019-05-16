/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/peer/peer_address.hpp"

#include "libp2p/multi/multiaddress_protocol_list.hpp"

namespace {
  using libp2p::multi::Protocol;
  using libp2p::multi::ProtocolList;

  const std::string kP2PSubstr =
      "/" + std::string{ProtocolList::get(Protocol::Code::P2P)->name} + "/";
}  // namespace

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::peer, PeerAddress::FactoryError, e) {
  using E = libp2p::peer::PeerAddress::FactoryError;
  switch (e) {
    case E::ID_EXPECTED:
      return "peer id not found in the address string";
    case E::SHA256_EXPECTED:
      return "peer id is not SHA-256 hash";
    case E::NO_ADDRESSES:
      return "no addresses in the provided PeerInfo";
  }
  return "unknown error";
}

namespace libp2p::peer {
  using multi::Multiaddress;
  using multi::Multihash;

  PeerAddress::PeerAddress(PeerId id, Multiaddress address)
      : id_{std::move(id)}, address_{std::move(address)} {}

  PeerAddress::FactoryResult PeerAddress::create(std::string_view address) {
    auto id_begin = address.find(kP2PSubstr);
    if (id_begin == std::string_view::npos) {
      return FactoryError::ID_EXPECTED;
    }

    auto address_str = address.substr(0, id_begin);
    OUTCOME_TRY(multiaddress, Multiaddress::create(address_str));

    auto id_b58_str = address.substr(id_begin + kP2PSubstr.size());
    OUTCOME_TRY(id, PeerId::fromBase58(id_b58_str));

    return PeerAddress{std::move(id), std::move(multiaddress)};
  }

  PeerAddress::FactoryResult PeerAddress::create(const PeerInfo &peer_info) {
    if (peer_info.addresses.empty()) {
      return FactoryError::NO_ADDRESSES;
    }

    return PeerAddress{peer_info.id, *peer_info.addresses.begin()};
  }

  PeerAddress::FactoryResult PeerAddress::create(const PeerId &peer_id,
                                                 const Multiaddress &address) {
    return PeerAddress{peer_id, address};
  }

  std::string PeerAddress::toString() const {
    return std::string{address_.getStringAddress()} + std::string{kP2PSubstr}
    + id_.toBase58();
  }

  const PeerId &PeerAddress::getId() const noexcept {
    return id_;
  }

  const multi::Multiaddress &PeerAddress::getAddress() const noexcept {
    return address_;
  }
}  // namespace libp2p::peer
