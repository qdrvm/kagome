/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_ROUTING_IMPL_HPP
#define KAGOME_ROUTING_IMPL_HPP

#include "libp2p/routing/routing_adaptor.hpp"

namespace libp2p::routing {
  /// stub for implementation of routing adaptor, remove or use as place, where
  /// the adaptor will be implemented
  class RoutingImpl : public RoutingAdaptor {
   public:
    ~RoutingImpl() override = default;
  };
}  // namespace libp2p::routing

#endif  // KAGOME_ROUTING_IMPL_HPP
