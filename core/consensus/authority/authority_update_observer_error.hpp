/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_AUTHORITIES_UPDATE_OBSERVER_ERROR
#define KAGOME_CONSENSUS_AUTHORITIES_UPDATE_OBSERVER_ERROR

#include <outcome/outcome.hpp>

namespace kagome::authority {

  enum class AuthorityUpdateObserverError {
    UNSUPPORTED_MESSAGE_TYPE = 1,
    WRONG_AUTHORITY_INDEX,
    NO_SCHEDULED_CHANGE_APPLIED_YET,
    NO_FORCED_CHANGE_APPLIED_YET,
    NO_PAUSE_APPLIED_YET,
    NO_RESUME_APPLIED_YET,
  };

}  // namespace kagome::authority

OUTCOME_HPP_DECLARE_ERROR(kagome::authority, AuthorityUpdateObserverError)

#endif  // KAGOME_CONSENSUS_AUTHORITIES_UPDATE_OBSERVER_ERROR
