/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_DHT_ADAPTOR_HPP
#define KAGOME_DHT_ADAPTOR_HPP

namespace libp2p::dht {
  /**
   * Distributed Hash Table
   */
  struct DHTAdaptor {
    virtual ~DHTAdaptor() = default;
  };
}  // namespace libp2p::dht

#endif  // KAGOME_DHT_ADAPTOR_HPP
