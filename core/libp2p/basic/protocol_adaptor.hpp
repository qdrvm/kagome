/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PROTOCOL_ADAPTOR_HPP
#define KAGOME_PROTOCOL_ADAPTOR_HPP

#include <memory>

#include "libp2p/basic/readwritecloser.hpp"
#include "libp2p/peer/protocol.hpp"

namespace libp2p::basic {
  /**
   * Encapsulates strategy of the given protocol
   */
  struct ProtocolAdaptor {
    virtual ~ProtocolAdaptor() = default;

    /**
     * Get a protocol, which is behind this adaptor
     * @return protocol id
     */
    virtual peer::Protocol getProtocolId() const = 0;

    /**
     * Make an action, which is specific for this adaptor, for example, secure
     * the connection in case this is implemented by some secure adaptor
     * @param c - connection or stream, over which the action is to be performed
     */
    virtual void operator()(std::shared_ptr<basic::ReadWriteCloser> c) = 0;
  };
}  // namespace libp2p::basic

#endif  // KAGOME_PROTOCOL_ADAPTOR_HPP
