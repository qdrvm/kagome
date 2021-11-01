/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_TRANSACTIONSTRANSMITTERMOCK
#define KAGOME_NETWORK_TRANSACTIONSTRANSMITTERMOCK

#include "network/transactions_transmitter.hpp"

#include <gmock/gmock.h>

namespace kagome::network {

  class TransactionsTransmitterMock : public TransactionsTransmitter {
   public:
    MOCK_METHOD(void,
                propagateTransactions,
                (gsl::span<const primitives::Transaction>),
                (override));
  };

}  // namespace kagome::network

#endif  // KAGOME_NETWORK_TRANSACTIONSTRANSMITTERMOCK
