/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/peer/peer_identity.hpp"

#include "libp2p/multi/multibase_codec/multibase_codec_impl.hpp"

namespace {
  constexpr std::string_view kIdSubstr = "/id/";

  const libp2p::multi::MultibaseCodecImpl kCodec{};
}  // namespace

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::peer, PeerIdentity::FactoryError, e) {
  using E = libp2p::peer::PeerIdentity::FactoryError;
  switch (e) {
    case E::ID_EXPECTED:
      return "peer id not found in the identity string";
    case E::SHA256_EXPECTED:
      return "peer id is not SHA-256 hash";
    case E::NO_ADDRESSES:
      return "no addresses in the provided PeerInfo";
  }
  return "unknown error";
}

namespace libp2p::peer {
  using multi::Multiaddress;
  using multi::MultibaseCodec;
  using multi::Multihash;

  PeerIdentity::PeerIdentity(PeerId id, Multiaddress address)
      : id_{std::move(id)}, address_{std::move(address)} {}

  PeerIdentity::FactoryResult PeerIdentity::create(std::string_view identity) {
    auto id_begin = identity.find(kIdSubstr);
    if (id_begin == std::string_view::npos) {
      return FactoryError::ID_EXPECTED;
    }

    auto address_str = identity.substr(0, id_begin);
    OUTCOME_TRY(address, Multiaddress::create(address_str));

    auto id_b58_str = identity.substr(id_begin + kIdSubstr.size());
    OUTCOME_TRY(id, PeerId::fromBase58(id_b58_str));

    return PeerIdentity{std::move(id), std::move(address)};
  }

  PeerIdentity::FactoryResult PeerIdentity::create(const PeerInfo &peer_info) {
    if (peer_info.addresses.empty()) {
      return FactoryError::NO_ADDRESSES;
    }

    return PeerIdentity{peer_info.id, *peer_info.addresses.begin()};
  }

  PeerIdentity::FactoryResult PeerIdentity::create(
      const PeerId &peer_id, const Multiaddress &address) {
    return PeerIdentity{peer_id, address};
  }

  std::string PeerIdentity::getIdentity() const {
    return std::string{address_.getStringAddress()} + std::string{kIdSubstr}
    + id_.toBase58();
  }

  const PeerId &PeerIdentity::getId() const noexcept {
    return id_;
  }

  const multi::Multiaddress &PeerIdentity::getAddress() const noexcept {
    return address_;
  }
}  // namespace libp2p::peer
