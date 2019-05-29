/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "transport_manager_impl.hpp"

#include <algorithm>

namespace libp2p::network {
  TransportManagerImpl::TransportManagerImpl() = default;

  TransportManagerImpl::TransportManagerImpl(
      std::vector<TransportSP> transports)
      : transports_{std::move(transports)} {}

  void TransportManagerImpl::add(TransportSP t) {
    transports_.push_back(std::move(t));
  }

  void TransportManagerImpl::add(gsl::span<TransportSP> t) {
    transports_.insert(transports_.end(), t.begin(), t.end());
  }

  gsl::span<TransportManagerImpl::TransportSP> TransportManagerImpl::getAll()
      const {
    return transports_;
  }

  void TransportManagerImpl::clear() {
    transports_.clear();
  }

  TransportManagerImpl::TransportSP TransportManagerImpl::findBest(
      const multi::Multiaddress &ma) {
    auto it = std::find_if(transports_.begin(), transports_.end(),
                           [&ma](const auto &t) { return t->canDial(ma); });
    if (it != transports_.end()) {
      return *it;
    }
    return nullptr;
  }
}  // namespace libp2p::network
