/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "scale/libp2p_types.hpp"

#include <algorithm>
#include <exception>

#include "scale/kagome_scale.hpp"

namespace scale {

  PeerInfoSerializable::PeerInfoSerializable()
      : PeerInfo{.id = dummyPeerId(), .addresses = {}} {}

  libp2p::peer::PeerId PeerInfoSerializable::dummyPeerId() {
    // some valid dummy peer id
    auto res = libp2p::peer::PeerId::fromBase58(
        "12D3KooWFN2mhgpkJsDBuNuE5427AcDrsib8EoqGMZmkxWwx3Md4");
    BOOST_ASSERT(res);
    return res.value();
  }

  void encode(const libp2p::peer::PeerInfo &peer_info,
              scale::Encoder &encoder) {
    std::vector<std::string> addresses;
    addresses.reserve(peer_info.addresses.size());
    for (const auto &addr : peer_info.addresses) {
      addresses.emplace_back(addr.getStringAddress());
    }
    encoder << peer_info.id.toBase58() << addresses;
  }

  void decode(libp2p::peer::PeerInfo &peer_info, scale::Decoder &decoder) {
    std::string peer_id_base58;
    std::vector<std::string> addresses;
    decoder >> peer_id_base58 >> addresses;
    auto peer_id_res = libp2p::peer::PeerId::fromBase58(peer_id_base58);
    peer_info.id = std::move(peer_id_res.value());
    std::vector<libp2p::multi::Multiaddress> multi_addrs;
    multi_addrs.reserve(addresses.size());
    std::ranges::for_each(addresses, [&multi_addrs](const auto &addr) {
      // filling in only supported kinds of addresses
      auto res = libp2p::multi::Multiaddress::create(addr);
      if (res) {
        multi_addrs.emplace_back(std::move(res.value()));
      }
    });
    peer_info.addresses = std::move(multi_addrs);
  }

}  // namespace scale
