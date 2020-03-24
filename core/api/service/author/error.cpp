/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::api, ExtrinsicApiError, e) {
  using kagome::api::ExtrinsicApiError;
  switch (e) {
    case ExtrinsicApiError::INVALID_STATE_TRANSACTION:
      return "transaction is in invalid state";
    case ExtrinsicApiError::UNKNOWN_STATE_TRANSACTION:
      return "transaction is in unknown state";
    case ExtrinsicApiError::DECODE_FAILURE:
      return "failed to decode value";
  }
  return "unknown author submission error";
}
