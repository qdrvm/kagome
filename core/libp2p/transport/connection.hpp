/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONNECTION_HPP
#define KAGOME_CONNECTION_HPP

#include <optional>
#include <vector>

#include "libp2p/basic/closeable.hpp"
#include "libp2p/basic/readable.hpp"
#include "libp2p/basic/writable.hpp"
#include "libp2p/multi/multiaddress.hpp"

namespace libp2p::transport {
  /**
   * Point-to-point link to the other peer
   */
  class Connection : public basic::Readable,
                     public basic::Writable,
                     public basic::Closeable {
   public:
    ~Connection() override = default;

    /**
     * This method retrieves the observed addresses we get from the underlying
     * transport, if any.
     * @return collection of such addresses
     */
    virtual outcome::result<multi::Multiaddress> getRemoteMultiaddr() const = 0;
  };
}  // namespace libp2p::transport

#endif  // KAGOME_CONNECTION_HPP
