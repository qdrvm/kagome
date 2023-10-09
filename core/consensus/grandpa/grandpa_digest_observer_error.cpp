/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "grandpa_digest_observer_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::consensus::grandpa,
                            GrandpaDigestObserverError,
                            e) {
  using E = kagome::consensus::grandpa::GrandpaDigestObserverError;
  switch (e) {
    case E::UNSUPPORTED_MESSAGE_TYPE:
      return "unsupported message type";
    case E::WRONG_AUTHORITY_INDEX:
      return "wrong authority index (out of bound)";
    case E::NO_SCHEDULED_CHANGE_APPLIED_YET:
      return "no previous change (scheduled) applied yet";
    case E::NO_FORCED_CHANGE_APPLIED_YET:
      return "no previous change (forced) applied yet";
    case E::NO_PAUSE_APPLIED_YET:
      return "no previous change (pause) applied yet";
    case E::NO_RESUME_APPLIED_YET:
      return "no previous change (resume) applied yet";
  }
  return "unknown error (invalid GrandpaDigestObserverError)";
}
