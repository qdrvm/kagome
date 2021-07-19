/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/transactions_transmitter_impl.hpp"

#include "network/router.hpp"

namespace kagome::network {

  TransactionsTransmitterImpl::TransactionsTransmitterImpl(
      std::shared_ptr<network::Router> router)
      : router_(std::move(router)) {}

  void TransactionsTransmitterImpl::propagateTransactions(
      gsl::span<const primitives::Transaction> txs) {
    auto protocol = router_->getPropagateTransactionsProtocol();
    BOOST_ASSERT_MSG(protocol,
                     "Router did not provide propagate transactions protocol");
    protocol->propagateTransactions(std::move(txs));
  }
}  // namespace kagome::network
