/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_DISCOVERY_IMPL_HPP
#define KAGOME_DISCOVERY_IMPL_HPP

#include "libp2p/discovery/discovery_adaptor.hpp"

namespace libp2p::discovery {
  /// stub for implementation of discovery adaptor, remove or use as place,
  /// where the adaptor will be implemented
  class DiscoveryImpl : public DiscoveryAdaptor {
   public:
    ~DiscoveryImpl() override = default;
  };
}  // namespace libp2p::discovery

#endif  // KAGOME_DISCOVERY_IMPL_HPP
