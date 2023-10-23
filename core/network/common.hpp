/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/peer/protocol.hpp>
#include <libp2p/peer/stream_protocols.hpp>

#include "application/chain_spec.hpp"
#include "blockchain/genesis_block_hash.hpp"

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
  const libp2p::peer::ProtocolName kLightProtocol = "/{}/light/2";
  const libp2p::peer::ProtocolName kCollationProtocol{"/{}/collation/1"};
  const libp2p::peer::ProtocolName kValidationProtocol{"/{}/validation/1"};
  const libp2p::peer::ProtocolName kReqCollationProtocol{"/{}/req_collation/1"};
  const libp2p::peer::ProtocolName kReqPovProtocol{"/{}/req_pov/1"};
  const libp2p::peer::ProtocolName kFetchChunkProtocol{"/{}/req_chunk/1"};
  const libp2p::peer::ProtocolName kFetchAvailableDataProtocol{
      "/{}/req_available_data/1"};
  const libp2p::peer::ProtocolName kFetchStatementProtocol{
      "/{}/req_statement/1"};
  const libp2p::peer::ProtocolName kSendDisputeProtocol{"/{}/send_dispute/1"};
  const libp2p::peer::ProtocolName kBeefyProtocol{"/{}/beefy/2"};
  const libp2p::peer::ProtocolName kBeefyJustificationProtocol{
      "/{}/beefy/justifications/1"};

  struct ProtocolPrefix {
    std::string_view prefix;
  };
  constexpr ProtocolPrefix kProtocolPrefixParitytech{"paritytech"};
  constexpr ProtocolPrefix kProtocolPrefixPolkadot{"polkadot"};

  template <typename... Args>
  libp2p::StreamProtocols make_protocols(std::string_view format,
                                         const Args &...args) {
    struct Visitor {
      std::string_view format;
      libp2p::StreamProtocols protocols;

      void visit(const blockchain::GenesisBlockHash &arg) {
        auto x = hex_lower(arg);
        protocols.emplace_back(fmt::vformat(format, fmt::make_format_args(x)));
      }
      void visit(const application::ChainSpec &arg) {
        protocols.emplace_back(
            fmt::vformat(format, fmt::make_format_args(arg.protocolId())));
      }
      void visit(const ProtocolPrefix &arg) {
        protocols.emplace_back(
            fmt::vformat(format, fmt::make_format_args(arg.prefix)));
      }
    } visitor{format, {}};
    (visitor.visit(args), ...);
    return visitor.protocols;
  }

}  // namespace kagome::network
