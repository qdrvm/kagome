/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_CONNECTION_HPP
#define KAGOME_NETWORK_CONNECTION_HPP

#include "libp2p/multi/multiaddress.hpp"
#include "libp2p/security/secure_connection.hpp"
#include "libp2p/stream/stream.hpp"

namespace libp2p::network {

  struct Connection : public security::SecureConnection {
    virtual ~Connection() = default;

    using ptr_t = std::shared_ptr<stream::Stream>;

    virtual multi::Multiaddress localMultiaddr() const = 0;

    virtual multi::Multiaddress remoteMultiaddr() const = 0;

    // open new stream on this connection
    virtual outcome::result<ptr_t> newStream() = 0;

    // get all streams associated with this connection
    virtual std::vector<ptr_t> getStreams() const = 0;
  };

}  // namespace libp2p::network

#endif  // KAGOME_NETWORK_CONNECTION_HPP
