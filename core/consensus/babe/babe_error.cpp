/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/babe/babe_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::consensus, BabeError, e) {
  using E = kagome::consensus::BabeError;
  switch (e) {
    case E::TIMER_ERROR:
      return "some internal error happened while using the timer in BABE; "
             "please, see logs";
    case E::NODE_FALL_BEHIND:
      return "local node has fallen too far behind the others, most likely "
             "it is in one of the previous epochs";
    case E::MISSING_PROOF:
      return "required VRF proof is missing";
  }
  return "unknown error";
}
