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
  class Identify : public std::enable_shared_from_this<Identify> {
    using HostSPtr = std::shared_ptr<Host>;
    using StreamSPtr = std::shared_ptr<connection::Stream>;

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
    void sendIdentify(StreamSPtr stream);

    /**
     * Called, when an identify message is written to the stream
     * @param written_bytes - how much bytes were written
     * @param stream with the other side
     * @param remote_address, to which it was written
     */
    void identifySent(outcome::result<size_t> written_bytes,
                      const StreamSPtr &stream, std::string &&remote_address);

    /**
     * Handler for the case, when we want to identify the other side
     * @param stream to be identified over
     */
    void receiveIdentify(StreamSPtr stream);

    /**
     * Called, when an identify message is received from the other peer
     * @param stream, over which it was received
     */
    void identifyReceived(StreamSPtr stream);

    HostSPtr host_;
    kagome::common::Logger log_;
  };
}  // namespace libp2p::protocol

#endif  // KAGOME_IDENTIFY_IMPL_HPP
