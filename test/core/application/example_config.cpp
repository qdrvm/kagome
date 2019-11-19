/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "core/application/example_config.hpp"


#include <boost/filesystem.hpp>

using kagome::common::unhex;
using kagome::crypto::ED25519PublicKey;
using kagome::crypto::SR25519PublicKey;
using libp2p::multi::Multiaddress;
using libp2p::peer::PeerId;
using libp2p::peer::PeerInfo;

namespace test::application {

  const KagomeConfig &getExampleConfig() {
    static bool init = true;
    static KagomeConfig c;
    if (init) {
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
      init = false;
    }
    return c;
  }

  std::stringstream readJSONConfig() {
    std::ifstream f(boost::filesystem::path(__FILE__).parent_path().string()
                    + "/example_config.json");
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
}  // namespace test
