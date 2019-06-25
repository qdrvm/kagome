/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "extrinsics_submission_service/error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::service, ExtrinsicSubmissionError, e) {
  using kagome::service::ExtrinsicSubmissionError;
  switch (e) {
    case ExtrinsicSubmissionError::INVALID_STATE_TRANSACTION:
      return "transaction is in invalid state";
    case ExtrinsicSubmissionError::UNKNOWN_STATE_TRANSACTION:
      return "transaction is in unknown state";
    case ExtrinsicSubmissionError::DECODE_FAILURE:
      return "failed to decode value";
  }
  return "unknown extrinsic submission error";
}
