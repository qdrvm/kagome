/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/peer/identity/peer_identity_factory_impl.hpp"

namespace {
  constexpr std::string_view kIdSubstr = "/id/";
}

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::peer, PeerIdentityFactoryImpl::FactoryError,
                            e) {
  using E = libp2p::peer::PeerIdentityFactoryImpl::FactoryError;
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

  PeerIdentityFactoryImpl::PeerIdentityFactoryImpl(
      std::shared_ptr<multi::MultibaseCodec> codec)
      : codec_{std::move(codec)} {}

  PeerIdentityFactoryImpl::FactoryResult PeerIdentityFactoryImpl::create(
      std::string_view identity) const {
    auto id_begin = identity.find(kIdSubstr);
    if (id_begin == std::string_view::npos) {
      return FactoryError::ID_EXPECTED;
    }

    auto address_str = identity.substr(0, id_begin);
    OUTCOME_TRY(address, Multiaddress::create(address_str));

    auto id_b58_str = identity.substr(id_begin + kIdSubstr.size());
    OUTCOME_TRY(id_bytes, codec_->decode(id_b58_str));
    OUTCOME_TRY(id_hash, Multihash::createFromBuffer(id_bytes.toVector()));

    return PeerIdentity{std::string{identity}, std::move(id_hash),
                        std::move(address)};
  }

  PeerIdentityFactoryImpl::FactoryResult PeerIdentityFactoryImpl::create(
      const PeerInfo &peer_info) const {
    if (peer_info.id.getType() != multi::HashType::sha256) {
      return FactoryError::SHA256_EXPECTED;
    }

    if (peer_info.addresses.empty()) {
      return FactoryError::NO_ADDRESSES;
    }

    const auto &address = *peer_info.addresses.begin();
    auto identity_str = std::string{address.getStringAddress()}
        + std::string{kIdSubstr}
        + codec_->encode(peer_info.id.toBuffer(),
                         MultibaseCodec::Encoding::BASE58);

    return PeerIdentity{std::move(identity_str), peer_info.id, address};
  }

  PeerIdentityFactoryImpl::FactoryResult PeerIdentityFactoryImpl::create(
      const PeerId &peer_id, const Multiaddress &address) const {
    if (peer_id.getType() != multi::HashType::sha256) {
      return FactoryError::SHA256_EXPECTED;
    }

    auto identity_str = std::string{address.getStringAddress()}
        + std::string{kIdSubstr}
        + codec_->encode(peer_id.toBuffer(), MultibaseCodec::Encoding::BASE58);

    return PeerIdentity{std::move(identity_str), peer_id, address};
  }
}  // namespace libp2p::peer
