/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_ID_UTILS_HPP
#define KAGOME_ID_UTILS_HPP

#include <memory>
#include <string>
#include <tuple>

#include <outcome/outcome.hpp>
#include "libp2p/connection/stream.hpp"
#include "libp2p/multi/multiaddress.hpp"

namespace libp2p::protocol::detail {
  /**
   * Get a tuple of stringified <PeerId, Multiaddress> of the peer the (\param
   * stream) is connected to
   */
  std::tuple<std::string, std::string> getPeerIdentity(
      const std::shared_ptr<libp2p::connection::Stream> &stream) {
    std::string id = "unknown";
    std::string addr = "unknown";
    if (auto id_res = stream->remotePeerId()) {
      id = id_res.value().toBase58();
    }
    if (auto addr_res = stream->remoteMultiaddr()) {
      addr = addr_res.value().getStringAddress();
    }
    return {std::move(id), std::move(addr)};
  }
}  // namespace libp2p::protocol::detail

#endif  // KAGOME_UTILS_HPP
