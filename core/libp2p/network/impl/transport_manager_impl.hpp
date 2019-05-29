/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TRANSPORT_MANAGER_IMPL_HPP
#define KAGOME_TRANSPORT_MANAGER_IMPL_HPP

#include <vector>

#include "libp2p/network/transport_manager.hpp"

namespace libp2p::network {
  class TransportManagerImpl : public TransportManager {
   public:
    /**
     * Initialize a transport manager with no supported transports
     */
    TransportManagerImpl();

    /**
     * Initialize a transport manager from a collection of transports
     * @param transports, which this manager is going to support
     */
    explicit TransportManagerImpl(std::vector<TransportSP> transports);

    ~TransportManagerImpl() override = default;

    void add(TransportSP t) override;

    void add(gsl::span<TransportSP> t) override;

    gsl::span<TransportSP> getAll() const override;

    void clear() override;

    TransportSP findBest(const multi::Multiaddress &ma) override;

   private:
    std::vector<TransportSP> transports_;
  };
}  // namespace libp2p::network

#endif  // KAGOME_TRANSPORT_MANAGER_IMPL_HPP
