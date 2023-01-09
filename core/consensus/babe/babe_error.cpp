/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/babe/babe_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::consensus::babe, BabeError, e) {
  using E = kagome::consensus::babe::BabeError;
  switch (e) {
    case E::MISSING_PROOF:
      return "required VRF proof is missing";
    case E::BAD_ORDER_OF_DIGEST_ITEM:
      return "bad order of digest item; PreRuntime must be first";
    case E::UNKNOWN_DIGEST_TYPE:
      return "unknown type of digest";
  }
  return "unknown error";
}
