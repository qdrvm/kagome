/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_COMMON_HPP
#define KAGOME_NETWORK_COMMON_HPP

#include <libp2p/peer/protocol.hpp>
#include <libp2p/peer/stream_protocols.hpp>

#include "application/chain_spec.hpp"
#include "primitives/common.hpp"

namespace kagome::network {
  /// Current protocol version.
  static constexpr uint32_t CURRENT_VERSION = 6;
  /// Lowest version we support
  static constexpr uint32_t MIN_VERSION = 3;

  const libp2p::peer::ProtocolName kStateProtocol = "/{}/state/2";
  const libp2p::peer::ProtocolName kSyncProtocol = "/{}/sync/2";
  const libp2p::peer::ProtocolName kPropagateTransactionsProtocol =
      "/{}/transactions/1";
  const libp2p::peer::ProtocolName kBlockAnnouncesProtocol =
      "/{}/block-announces/1";
  const libp2p::peer::ProtocolName kGrandpaProtocol = "/{}/grandpa/1";
  const libp2p::peer::ProtocolName kWarpProtocol = "/{}/sync/warp";
  const libp2p::peer::ProtocolName kCollationProtocol{"/{}/collation/1"};
  const libp2p::peer::ProtocolName kValidationProtocol{"/{}/validation/1"};
  const libp2p::peer::ProtocolName kReqCollationProtocol{"/{}/req_collation/1"};

  template <typename... Args>
  libp2p::StreamProtocols make_protocols(std::string_view format,
                                         const Args &...args) {
    libp2p::StreamProtocols protocols;
    auto instantiate = [&](const auto &arg) {
      if constexpr (std::is_same_v<std::decay_t<decltype(arg)>,
                                   std::decay_t<primitives::BlockHash>>) {
        protocols.emplace_back(fmt::format(format, hex_lower(arg)));
      } else if constexpr (std::is_same_v<
                               std::decay_t<decltype(arg)>,
                               std::decay_t<application::ChainSpec>>) {
        protocols.emplace_back(fmt::format(format, arg.protocolId()));
      } else {
        protocols.emplace_back(fmt::format(format, arg));
      }
    };
    (instantiate(args), ...);
    return protocols;
  }

}  // namespace kagome::network

#endif  // KAGOME_NETWORK_COMMON_HPP
