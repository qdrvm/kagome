/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_ADDRESS_REPOSITORY_HPP
#define KAGOME_ADDRESS_REPOSITORY_HPP

#include <chrono>

#include <gsl/span>
#include "libp2p/multi/multiaddress.hpp"
#include "libp2p/peer/peer_id.hpp"

namespace libp2p::peer {

  class AddressRepository {
    using Milliseconds = std::chrono::milliseconds;

   public:
    virtual ~AddressRepository() = default;

    virtual void addAddress(const PeerId &p, const multi::Multiaddress &ma,
                            Milliseconds ttl) = 0;

    virtual void addAddresses(const PeerId &p,
                              gsl::span<multi::Multiaddress> span,
                              Milliseconds ttl) = 0;

    virtual void clearAddresses(const PeerId &p) = 0;
  };

}  // namespace libp2p::peer

#endif  //KAGOME_ADDRESS_REPOSITORY_HPP
