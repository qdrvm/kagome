/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_EXTRINSICS_SUBMISSION_SERVICE_ERROR_HPP
#define KAGOME_CORE_EXTRINSICS_SUBMISSION_SERVICE_ERROR_HPP

#include <outcome/outcome.hpp>

namespace kagome::api {
  enum class ExtrinsicApiError {
    INVALID_STATE_TRANSACTION = 1,  // transaction is in invalid state
    UNKNOWN_STATE_TRANSACTION,      // transaction is in unknown state
    DECODE_FAILURE                  // failed to decode value
  };
}

OUTCOME_HPP_DECLARE_ERROR(kagome::api, ExtrinsicApiError)

#endif  // KAGOME_CORE_EXTRINSICS_SUBMISSION_SERVICE_ERROR_HPP
