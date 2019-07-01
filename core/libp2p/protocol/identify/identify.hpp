/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_IDENTIFY_IMPL_HPP
#define KAGOME_IDENTIFY_IMPL_HPP

#include <memory>

#include "common/logger.hpp"
#include "libp2p/connection/stream.hpp"
#include "libp2p/host.hpp"

namespace libp2p::protocol {
  /**
   * Implementation of an Identify protocol, which is a way to say
   * "hello" to the other peer, sending our listen addresses, ID, etc
   * Read more: https://github.com/libp2p/specs/tree/master/identify
   */
  class Identify {
    using HostSPtr = std::shared_ptr<Host>;

   public:
    explicit Identify(
        HostSPtr host,
        kagome::common::Logger log = kagome::common::createLogger("Identify"));

   private:
    /**
     * Handler for the case, when we are being identified by the other peer; we
     * should respond with an Identify message and close the stream
     * @param stream to be identified over
     */
    void identify(std::shared_ptr<connection::Stream> stream);

    HostSPtr host_;

    kagome::common::Logger log_;
  };
}  // namespace libp2p::protocol

#endif  // KAGOME_IDENTIFY_IMPL_HPP
