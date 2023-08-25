/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "outcome/outcome.hpp"

namespace kagome::consensus::grandpa {

  enum class GrandpaDigestObserverError {
    UNSUPPORTED_MESSAGE_TYPE = 1,
    WRONG_AUTHORITY_INDEX,
    NO_SCHEDULED_CHANGE_APPLIED_YET,
    NO_FORCED_CHANGE_APPLIED_YET,
    NO_PAUSE_APPLIED_YET,
    NO_RESUME_APPLIED_YET,
  };

}  // namespace kagome::consensus::grandpa

OUTCOME_HPP_DECLARE_ERROR(kagome::consensus::grandpa,
                          GrandpaDigestObserverError)
