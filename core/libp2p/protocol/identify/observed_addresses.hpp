/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_OBSERVED_ADDRESSES_HPP
#define KAGOME_OBSERVED_ADDRESSES_HPP

#include <vector>

#include "libp2p/multi/multiaddress.hpp"

namespace libp2p::protocol {
  class ObservedAddresses {
   public:
    std::vector<multi::Multiaddress> getAllAddresses() const;

    std::vector<multi::Multiaddress> getAddressesFor(
        const multi::Multiaddress &address) const;

    void add(multi::Multiaddress observed, multi::Multiaddress local,
             multi::Multiaddress observer, bool is_initiator);
  };
}  // namespace libp2p::protocol

#endif  // KAGOME_OBSERVED_ADDRESSES_HPP
