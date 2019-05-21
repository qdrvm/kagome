/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NEGOTIATOR_HPP
#define KAGOME_NEGOTIATOR_HPP

#include <memory>

#include "libp2p/basic/readwritecloser.hpp"
#include "libp2p/peer/protocol.hpp"
#include "libp2p/connection/stream.hpp"

namespace libp2p::network {

  /**
   * @brief Negotiator is a component capable of reaching agreement over what
   * protocols to use for inbound streams of communication.
   * @see
   * https://sourcegraph.com/github.com/libp2p/go-libp2p-core@consolidate-skeleton/-/blob/protocol/switch.go#L53
   */
  struct Negotiator {
    virtual ~Negotiator() = default;

    // TODO: come up with better name
    struct Return {
      peer::Protocol protocol;
      std::function<connection::Stream::Handler> handler;
    };

    // NegotiateLazy will return the registered protocol handler to use
    // for a given inbound stream, returning as soon as the protocol has been
    // determined. Returns an error if negotiation fails.
    //
    // NegotiateLazy may return before all protocol negotiation responses have
    // been written to the stream. This is in contrast to Negotiate, which will
    // block until the Negotiator is finished with the stream.
    virtual outcome::result<Return> negotiateLazy(
        std::shared_ptr<basic::ReadWriteCloser> io) = 0;

    // Negotiate will return the registered protocol handler to use for a given
    // inbound stream, returning after the protocol has been determined and the
    // Negotiator has finished using the stream for negotiation. Returns an
    // error if negotiation fails.
    virtual outcome::result<Return> negotiate(
        std::shared_ptr<basic::ReadWriteCloser> io) = 0;

    // Handle calls Negotiate to determine which protocol handler to use for an
    // inbound stream, then invokes the protocol handler function, passing it
    // the protocol ID and the stream. Returns an error if negotiation fails.
    virtual outcome::result<void> handle(
        std::shared_ptr<basic::ReadWriteCloser> io) = 0;
  };

}  // namespace libp2p::network

#endif  // KAGOME_NEGOTIATOR_HPP
