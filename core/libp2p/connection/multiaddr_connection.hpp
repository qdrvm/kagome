/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_MULTIADDR_CONNECTION_HPP
#define KAGOME_MULTIADDR_CONNECTION_HPP

#include "libp2p/multi/multiaddress.hpp"

namespace libp2p::connection {

  /// connection, which has assigned multiaddresses
  struct MultiaddrConnection {
    virtual ~MultiaddrConnection() = default;
  };

}  // namespace libp2p::connection

#endif  // KAGOME_MULTIADDR_CONNECTION_HPP
