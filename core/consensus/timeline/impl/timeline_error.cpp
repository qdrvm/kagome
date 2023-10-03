/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/timeline/impl/timeline_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::consensus, TimelineError, e) {
  using E = kagome::consensus::TimelineError;
  switch (e) {
    case E::SLOT_BEFORE_GENESIS:
      return "slot before genesis";
  }
  return "unknown error (kagome::consensus::TimelineError)";
}
