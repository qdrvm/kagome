/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <chrono>

#include <libp2p/peer/peer_id.hpp>

namespace kagome::network {

  using Reputation = std::int32_t;

  struct ReputationChange {
   public:
    const Reputation value;
    const std::string_view reason;

    ReputationChange operator+(Reputation delta) const {
      if (delta < 0) {
        if (value < 0 and value + delta > 0) {  // negative overflow
          return {std::numeric_limits<Reputation>::min(), reason};
        }
      } else if (delta > 0) {
        if (value > 0 and value + delta < 0) {  // positive overflow
          return {std::numeric_limits<Reputation>::max(), reason};
        }
      }
      return {value + delta, reason};
    }

    ReputationChange operator*(size_t times) const {
      auto limit = value < 0 ? std::numeric_limits<Reputation>::min()
                             : std::numeric_limits<Reputation>::max();
      if (static_cast<size_t>(limit / value) < times) {  // overflow
        return {limit, reason};
      }
      return {static_cast<Reputation>(value * times), reason};
    }
  };

  namespace reputation {

    // clang-format off
    namespace cost {
      const ReputationChange UNEXPECTED_DISCONNECT        = {.value = -100, .reason = "Network: Unexpected disconnect"};
      const ReputationChange DUPLICATE_BLOCK_REQUEST      = {.value = -100, .reason = "Sync: Duplicate block request"};

      const ReputationChange PAST_REJECTION               = {.value = -50,   .reason = "Grandpa: Past message"};

      const ReputationChange BAD_SIGNATURE                = {.value = -100,  .reason = "Grandpa: Bad signature"};
      const ReputationChange MALFORMED_CATCH_UP           = {.value = -1000, .reason = "Grandpa: Malformed catch-up"};
      const ReputationChange MALFORMED_COMMIT             = {.value = -1000, .reason = "Grandpa: Malformed commit"};

      // A message received that's from the future relative to our view. always misbehavior.
      const ReputationChange FUTURE_MESSAGE               = {.value = -500,  .reason = "Grandpa: Future message"};

      const ReputationChange UNKNOWN_VOTER                = {.value = -150,  .reason = "Grandpa: Unknown voter"};

      // invalid neighbor message, considering the last one
      const ReputationChange INVALID_VIEW_CHANGE          = {.value = -500,  .reason = "Grandpa: Invalid view change"};

      // could not decode neighbor message. bytes-length of the packet.
      // TODO need to catch scale-decoder error for that
      const ReputationChange UNDECODABLE_NEIGHBOR_MESSAGE = {.value = -5,    .reason = "Grandpa: Bad packet"};

      const ReputationChange PER_UNDECODABLE_BYTE         = {.value = -5,    .reason = ""};

      // bad signature in catch-up response (for each checked signature)
      const ReputationChange BAD_CATCHUP_RESPONSE         = {.value = -25,   .reason = "Grandpa: Bad catch-up message"};

      const ReputationChange PER_SIGNATURE_CHECKED        = {.value = -25,   .reason = ""};

      const ReputationChange PER_BLOCK_LOADED             = {.value = -10,   .reason = ""};
      const ReputationChange INVALID_CATCH_UP             = {.value = -5000, .reason = "Grandpa: Invalid catch-up"};
      const ReputationChange INVALID_COMMIT               = {.value = -5000, .reason = "Grandpa: Invalid commit"};
      const ReputationChange OUT_OF_SCOPE_MESSAGE         = {.value = -500,  .reason = "Grandpa: Out-of-scope message"};
      const ReputationChange CATCH_UP_REQUEST_TIMEOUT     = {.value = -200,  .reason = "Grandpa: Catch-up request timeout"};

      // cost of answering a catch-up request
      const ReputationChange CATCH_UP_REPLY               = {.value = -200,  .reason = "Grandpa: Catch-up reply"};

      // A message received that cannot be evaluated relative to our view.
      // This happens before we have a view and have sent out neighbor packets.
      // always misbehavior.
      const ReputationChange HONEST_OUT_OF_SCOPE_CATCH_UP = {.value = -200,  .reason = "Grandpa: Out-of-scope catch-up"};

      // Dispute distribution penalties
      const ReputationChange INVALID_DISPUTE_REQUEST      = {.value = -100, .reason = "Dispute: Received message could not be decoded"};
      const ReputationChange INVALID_SIGNATURE_DISPUTE    = {.value = std::numeric_limits<Reputation>::min(), .reason = "Dispute: Signatures were invalid"};
      const ReputationChange NOT_A_VALIDATOR_DISPUTE      = {.value = -300, .reason = "Dispute: Reporting peer was not a validator"};
      const ReputationChange INVALID_IMPORT_DISPUTE       = {.value = -100, .reason = "Dispute: Import was deemed invalid by dispute-coordinator"};
      const ReputationChange APPARENT_FLOOD_DISPUTE       = {.value = -100, .reason = "Dispute: Peer exceeded the rate limit"};

    }  // namespace cost

    namespace benefit {
      const ReputationChange NEIGHBOR_MESSAGE             = {.value = 100,   .reason = "Grandpa: Neighbor message"};
      const ReputationChange ROUND_MESSAGE                = {.value = 100,   .reason = "Grandpa: Round message"};
      const ReputationChange BASIC_VALIDATED_CATCH_UP     = {.value = 200,   .reason = "Grandpa: Catch-up message"};
      const ReputationChange BASIC_VALIDATED_COMMIT       = {.value = 100,   .reason = "Grandpa: Commit"};
      const ReputationChange PER_EQUIVOCATION             = {.value = 10,    .reason = ""};
    }  // namespace benefit
    // clang-format on

  }  // namespace reputation

}  // namespace kagome::network
