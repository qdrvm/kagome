/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_UPGRADER_HPP
#define KAGOME_UPGRADER_HPP

#include <memory>

#include <outcome/outcome.hpp>
#include "libp2p/connection/capable_connection.hpp"
#include "libp2p/connection/raw_connection.hpp"
#include "libp2p/connection/secure_connection.hpp"

namespace libp2p::transport {

  /**
   * Lifetime of the connection is: Raw -> Secure -> Muxed -> [Streams over
   * Muxed]. Upgrader handles the first two steps, understanding, which security
   * and muxer protocols are available on both sides (via Multiselect protocol)
   * and using the chosen protocols to actually upgrade the connections
   */
  struct Upgrader {
    virtual ~Upgrader() = default;

    /**
     * Upgrade a raw connection to the secure one
     * @param conn to be upgraded
     * @return upgraded connection or error
     */
    virtual outcome::result<std::shared_ptr<connection::SecureConnection>>
    upgradeToSecure(std::shared_ptr<connection::RawConnection> conn) = 0;

    /**
     * Upgrade a secure connection to the muxed (capable) one
     * @param conn to be upgraded
     * @return upgraded connection or error
     */
    virtual outcome::result<std::shared_ptr<connection::CapableConnection>>
    upgradeToMuxed(std::shared_ptr<connection::SecureConnection> conn) = 0;
  };

}  // namespace libp2p::transport

#endif  // KAGOME_UPGRADER_HPP
