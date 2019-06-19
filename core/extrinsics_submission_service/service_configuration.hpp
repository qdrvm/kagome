/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_EXTRINSICS_SUBMISSION_SERVICE_SERVICE_CONFIGURATION_HPP
#define KAGOME_CORE_EXTRINSICS_SUBMISSION_SERVICE_SERVICE_CONFIGURATION_HPP

#include <cstdint>

namespace kagome::service {
  struct ExtrinsicSubmissionServiceConfiguration {
    uint32_t port;
  };
}  // namespace kagome::service

#endif  // KAGOME_CORE_EXTRINSICS_SUBMISSION_SERVICE_SERVICE_CONFIGURATION_HPP
