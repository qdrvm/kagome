/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PROTOCOL_MANAGER_HPP
#define KAGOME_PROTOCOL_MANAGER_HPP

#include "libp2p/peer/protocol.hpp"
#include "libp2p/stream/stream.hpp"

namespace libp2p::swarm {

  struct ProtocolManager {
    using StreamHandler = void(std::shared_ptr<stream::Stream>);

    virtual ~ProtocolManager() = default;

    virtual void setProtocolHandler(
        const peer::Protocol &protocol,
        const std::function<StreamHandler> &handler) = 0;

    virtual void setProtocolHandlerMatch(
        std::string_view prefix,
        const std::function<bool(const peer::Protocol &)> &predicate,
        const std::function<StreamHandler> &handler) = 0;

    virtual void invoke(const peer::Protocol &p,
                        std::shared_ptr<stream::Stream> stream) = 0;

    virtual void removeProtocolHandler(const peer::Protocol &protocol) = 0;

    virtual void removeAll() = 0;
  };

}  // namespace libp2p::swarm

#endif  // KAGOME_PROTOCOL_MANAGER_HPP
