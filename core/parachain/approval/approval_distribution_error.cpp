/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "parachain/approval/approval_distribution_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::parachain, ApprovalDistributionError, e) {
  using E = kagome::parachain::ApprovalDistributionError;
  switch (e) {
    case E::NO_INSTANCE:
      return "No self instance";
    case E::NO_CONTEXT:
      return "No context";
    case E::NO_SESSION_INFO:
      return "No session info";
    case E::UNUSED_SLOT_TYPE:
      return "Unused slot type";
    case E::ENTRY_IS_NOT_FOUND:
      return "Entry is not found";
    case E::ALREADY_APPROVING:
      return "Block in progress";
    case E::VALIDATOR_INDEX_OUT_OF_BOUNDS:
      return "Validator index out of bounds";
    case E::CORE_INDEX_OUT_OF_BOUNDS:
      return "Core index out of bounds";
    case E::CANDIDATE_INDEX_OUT_OF_BOUNDS:
      return "Candidate index out of bounds";
    case E::IS_IN_BACKING_GROUP:
      return "Is in backing group";
    case E::SAMPLE_OUT_OF_BOUNDS:
      return "Sample is out of bounds";
    case E::VRF_DELAY_CORE_INDEX_MISMATCH:
      return "VRF delay core index mismatch";
    case E::VRF_MODULO_CORE_INDEX_MISMATCH:
      return "VRF modulo core index mismatch";
    case E::INVALID_ARGUMENTS:
      return "Invalid arguments";
    case E::VRF_VERIFY_AND_GET_TRANCHE:
      return "VRF verify and get tranche failed";
  }
  return "Unknown approval-distribution error";
}
