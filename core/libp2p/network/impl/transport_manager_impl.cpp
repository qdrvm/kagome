/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "transport_manager_impl.hpp"

#include <algorithm>

namespace libp2p::network {
  TransportManagerImpl::TransportManagerImpl(
      std::vector<TransportSPtr> transports)
      : transports_{std::move(transports)} {}

  void TransportManagerImpl::add(TransportSPtr t) {
    transports_.push_back(std::move(t));
  }

  void TransportManagerImpl::add(gsl::span<const TransportSPtr> t) {
    transports_.insert(transports_.end(), t.begin(), t.end());
  }

  gsl::span<const TransportManagerImpl::TransportSPtr>
  TransportManagerImpl::getAll() const {
    return transports_;
  }

  void TransportManagerImpl::clear() {
    transports_.clear();
  }

  TransportManagerImpl::TransportSPtr TransportManagerImpl::findBest(
      const multi::Multiaddress &ma) {
    auto it = std::find_if(transports_.begin(), transports_.end(),
                           [&ma](const auto &t) { return t->canDial(ma); });
    if (it != transports_.end()) {
      return *it;
    }
    return nullptr;
  }
}  // namespace libp2p::network
