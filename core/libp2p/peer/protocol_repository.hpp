/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PROTOCOL_REPOSITORY_HPP
#define KAGOME_PROTOCOL_REPOSITORY_HPP

#include <list>

#include <gsl/span>
#include <outcome/outcome.hpp>
#include "libp2p/basic/garbage_collectable.hpp"
#include "libp2p/peer/peer_id.hpp"
#include "libp2p/peer/protocol.hpp"

namespace libp2p::peer {

  class ProtocolRepository : public basic::GarbageCollectable {
   public:
    ~ProtocolRepository() override = default;

    virtual outcome::result<void> addProtocols(
        const PeerId &p, gsl::span<const Protocol> ms) = 0;

    virtual outcome::result<void> removeProtocols(
        const PeerId &p, gsl::span<const Protocol> ms) = 0;

    virtual outcome::result<std::list<Protocol>> getProtocols(
        const PeerId &p) const = 0;

    virtual outcome::result<std::list<Protocol>> supportsProtocols(
        const PeerId &p, gsl::span<const Protocol> protocols) const = 0;

    virtual void clear(const PeerId &p) = 0;
  };

}  // namespace libp2p::peer

#endif  // KAGOME_PROTOCOL_REPOSITORY_HPP
