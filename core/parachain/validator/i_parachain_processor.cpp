/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "parachain/validator/i_parachain_processor.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::parachain,
                            ParachainProcessor::Error,
                            e) {
  using E = kagome::parachain::ParachainProcessor::Error;
  switch (e) {
    case E::RESPONSE_ALREADY_RECEIVED:
      return "Response already present";
    case E::REJECTED_BY_PROSPECTIVE_PARACHAINS:
      return "Rejected by prospective parachains";
    case E::COLLATION_NOT_FOUND:
      return "Collation not found";
    case E::UNDECLARED_COLLATOR:
      return "Undeclared collator";
    case E::KEY_NOT_PRESENT:
      return "Private key is not present";
    case E::VALIDATION_FAILED:
      return "Validate and make available failed";
    case E::VALIDATION_SKIPPED:
      return "Validate and make available skipped";
    case E::OUT_OF_VIEW:
      return "Out of view";
    case E::CORE_INDEX_UNAVAILABLE:
      return "Core index unavailable";
    case E::DUPLICATE:
      return "Duplicate";
    case E::NO_INSTANCE:
      return "No self instance";
    case E::NOT_A_VALIDATOR:
      return "Node is not a validator";
    case E::NOT_SYNCHRONIZED:
      return "Node not synchronized";
    case E::PEER_LIMIT_REACHED:
      return "Peer limit reached";
    case E::PROTOCOL_MISMATCH:
      return "Protocol mismatch";
    case E::NOT_CONFIRMED:
      return "Candidate not confirmed";
    case E::NO_STATE:
      return "No parachain state";
    case E::NO_SESSION_INFO:
      return "No session info";
    case E::OUT_OF_BOUND:
      return "Index out of bound";
    case E::INCORRECT_BITFIELD_SIZE:
      return "Incorrect bitfield size";
    case E::INCORRECT_SIGNATURE:
      return "Incorrect signature";
    case E::CLUSTER_TRACKER_ERROR:
      return "Cluster tracker error";
    case E::PERSISTED_VALIDATION_DATA_NOT_FOUND:
      return "Persisted validation data not found";
    case E::PERSISTED_VALIDATION_DATA_MISMATCH:
      return "Persisted validation data mismatch";
    case E::CANDIDATE_HASH_MISMATCH:
      return "Candidate hash mismatch";
    case E::PARENT_HEAD_DATA_MISMATCH:
      return "Parent head data mismatch";
    case E::NO_PEER:
      return "No peer";
    case E::ALREADY_REQUESTED:
      return "Already requested";
    case E::NOT_ADVERTISED:
      return "Not advertised";
    case E::WRONG_PARA:
      return "Wrong para id";
    case E::THRESHOLD_LIMIT_REACHED:
      return "Threshold reached";
  }
  return "Unknown parachain processor error";
}
