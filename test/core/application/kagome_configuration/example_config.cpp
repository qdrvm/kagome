/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "example_config.hpp"

#include <boost/filesystem.hpp>

namespace kagome::application {

  using common::unhex;
  using crypto::ED25519PublicKey;
  using crypto::SR25519PublicKey;
  using libp2p::multi::Multiaddress;
  using libp2p::peer::PeerId;
  using libp2p::peer::PeerInfo;

  const KagomeConfig &getExampleConfig() {
    static bool is_initialized = false;
    static KagomeConfig c;
    if (!is_initialized) {
      c.genesis.header.number = 42;
      c.api_ports.extrinsic_api_port = 4224;
      c.peers_info = {
          PeerInfo{PeerId::fromBase58(
                       "1AWR4A2YXCzotpPjJshv1QUwSTExoYWiwr33C4briAGpCY")
                       .value(),
                   {Multiaddress::create("/ip4/127.0.0.1/udp/1234").value(),
                    Multiaddress::create("/ipfs/mypeer").value()}},
          PeerInfo{PeerId::fromBase58(
                       "1AWUyTAqzDb7C3XpZP9DLKmpDDV81kBndfbSrifEkm29XF")
                       .value(),
                   {Multiaddress::create("/ip4/127.0.0.1/tcp/1020").value(),
                    Multiaddress::create("/ipfs/mypeer").value()}}};
      c.session_keys = {
          SR25519PublicKey::fromHex("010101010101010101010101010101010101010101"
                                    "0101010101010101010101")
              .value(),
          SR25519PublicKey::fromHex("020202020202020202020202020202020202020202"
                                    "0202020202020202020202")
              .value()};
      c.authorities = {
          ED25519PublicKey::fromHex("010101010101010101010101010101010101010101"
                                    "0101010101010101010101")
              .value(),
          ED25519PublicKey::fromHex("020202020202020202020202020202020202020202"
                                    "0202020202020202020202")
              .value()};
      is_initialized = true;
    }
    return c;
  }

  std::stringstream readJSONConfig() {
    auto path = boost::filesystem::path(__FILE__).parent_path().string()
                + "/example_config.json";
    std::ifstream f(path);
    if (!f) {
      throw std::runtime_error("config file not found");
    }
    std::stringstream ss;
    while (f) {
      std::string s;
      f >> s;
      ss << s;
    }
    return ss;
  }
}  // namespace kagome::application
