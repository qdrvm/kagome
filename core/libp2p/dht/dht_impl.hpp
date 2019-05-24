/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_DHT_IMPL_HPP
#define KAGOME_DHT_IMPL_HPP

#include "libp2p/dht/dht_adaptor.hpp"

namespace libp2p::dht {
  /// stub for implementation of DHT adaptor, remove or use as place,
  /// where the adaptor will be implemented
  class DHTImpl : public DHTAdaptor {
    ~DHTImpl() override = default;
  };
}  // namespace libp2p::dht

#endif  // KAGOME_DHT_IMPL_HPP
