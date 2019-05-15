/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TRANSPORT_MANAGER_HPP
#define KAGOME_TRANSPORT_MANAGER_HPP

#include "libp2p/multi/multiaddress.hpp"
#include "libp2p/transport/transport.hpp"

namespace libp2p::swarm {

  struct TransportManager {
    using t_id = multi::Protocol::Code;
    using sptrt =
        std::shared_ptr<transport::Transport>;  // transport must have getter
                                                // which returns t_id
    virtual ~TransportManager() = default;

    virtual void add(sptrt transport) = 0;

    virtual void remove(t_id id) = 0;

    virtual void clear() = 0;

    virtual sptrt get(t_id id) const = 0;

    virtual bool supports(t_id id) const = 0;

    virtual std::vector<sptrt> getAll() const = 0;
  };

}  // namespace libp2p::swarm

#endif  // KAGOME_TRANSPORT_MANAGER_HPP
