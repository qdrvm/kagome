/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_DISCOVERY_ADAPTOR_HPP
#define KAGOME_DISCOVERY_ADAPTOR_HPP

namespace libp2p::discovery {
  /**
   * Continuously scans the network to find and cache new peers
   */
  struct DiscoveryAdaptor {
    virtual ~DiscoveryAdaptor() = default;
  };
}  // namespace libp2p::discovery

#endif  // KAGOME_DISCOVERY_ADAPTOR_HPP
