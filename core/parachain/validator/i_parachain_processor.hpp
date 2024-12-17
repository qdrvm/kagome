/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "outcome/outcome.hpp"

namespace kagome::parachain {

  class ParachainProcessor {
   public:
    enum class Error {
      RESPONSE_ALREADY_RECEIVED = 1,
      COLLATION_NOT_FOUND,
      KEY_NOT_PRESENT,
      VALIDATION_FAILED,
      VALIDATION_SKIPPED,
      OUT_OF_VIEW,
      DUPLICATE,
      NO_INSTANCE,
      NOT_A_VALIDATOR,
      NOT_SYNCHRONIZED,
      UNDECLARED_COLLATOR,
      PEER_LIMIT_REACHED,
      PROTOCOL_MISMATCH,
      NOT_CONFIRMED,
      NO_STATE,
      NO_SESSION_INFO,
      OUT_OF_BOUND,
      REJECTED_BY_PROSPECTIVE_PARACHAINS,
      INCORRECT_BITFIELD_SIZE,
      CORE_INDEX_UNAVAILABLE,
      INCORRECT_SIGNATURE,
      CLUSTER_TRACKER_ERROR,
      PERSISTED_VALIDATION_DATA_NOT_FOUND,
      PERSISTED_VALIDATION_DATA_MISMATCH,
      CANDIDATE_HASH_MISMATCH,
      PARENT_HEAD_DATA_MISMATCH,
      NO_PEER,
      ALREADY_REQUESTED,
      NOT_ADVERTISED,
      WRONG_PARA,
      THRESHOLD_LIMIT_REACHED,
    };

    virtual ~ParachainProcessor() = default;

  };

}

OUTCOME_HPP_DECLARE_ERROR(kagome::parachain, ParachainProcessor::Error);
