/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/transaction_validity.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::primitives, TransactionValidityErrc, e) {
  using kagome::primitives::TransactionValidityErrc;
  switch (e) {
    case TransactionValidityErrc::INVALID:
      return "transaction is invalid";
    case TransactionValidityErrc::UNKNOWN:
      return "transaction validation error is unknown";
  }
}
