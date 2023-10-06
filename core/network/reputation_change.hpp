/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_REPUTATIONCHANGE
#define KAGOME_NETWORK_REPUTATIONCHANGE

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
      const ReputationChange UNEXPECTED_DISCONNECT        = {-1000, "Network: Unexpected disconnect"};
      const ReputationChange DUPLICATE_BLOCK_REQUEST      = {-50,   "Sync: Duplicate block request"};

      const ReputationChange PAST_REJECTION               = {-50,   "Grandpa: Past message"};

      const ReputationChange BAD_SIGNATURE                = {-100,  "Grandpa: Bad signature"};
      const ReputationChange MALFORMED_CATCH_UP           = {-1000, "Grandpa: Malformed catch-up"};
      const ReputationChange MALFORMED_COMMIT             = {-1000, "Grandpa: Malformed commit"};

      // A message received that's from the future relative to our view. always misbehavior.
      const ReputationChange FUTURE_MESSAGE               = {-500,  "Grandpa: Future message"};

      const ReputationChange UNKNOWN_VOTER                = {-150,  "Grandpa: Unknown voter"};

      // invalid neighbor message, considering the last one
      const ReputationChange INVALID_VIEW_CHANGE          = {-500,  "Grandpa: Invalid view change"};

      // could not decode neighbor message. bytes-length of the packet.
      // TODO need to catch scale-decoder error for that
      const ReputationChange UNDECODABLE_NEIGHBOR_MESSAGE = {-5,    "Grandpa: Bad packet"};

      const ReputationChange PER_UNDECODABLE_BYTE         = {-5,    ""};

      // bad signature in catch-up response (for each checked signature)
      const ReputationChange BAD_CATCHUP_RESPONSE         = {-25,   "Grandpa: Bad catch-up message"};

      const ReputationChange PER_SIGNATURE_CHECKED        = {-25,   ""};

      const ReputationChange PER_BLOCK_LOADED             = {-10,   ""};
      const ReputationChange INVALID_CATCH_UP             = {-5000, "Grandpa: Invalid catch-up"};
      const ReputationChange INVALID_COMMIT               = {-5000, "Grandpa: Invalid commit"};
      const ReputationChange OUT_OF_SCOPE_MESSAGE         = {-500,  "Grandpa: Out-of-scope message"};
      const ReputationChange CATCH_UP_REQUEST_TIMEOUT     = {-200,  "Grandpa: Catch-up request timeout"};

      // cost of answering a catch-up request
      const ReputationChange CATCH_UP_REPLY               = {-200,  "Grandpa: Catch-up reply"};

      // A message received that cannot be evaluated relative to our view.
      // This happens before we have a view and have sent out neighbor packets.
      // always misbehavior.
      const ReputationChange HONEST_OUT_OF_SCOPE_CATCH_UP = {-200,  "Grandpa: Out-of-scope catch-up"};

    }  // namespace cost

    namespace benefit {
      const ReputationChange NEIGHBOR_MESSAGE             = {100,   "Grandpa: Neighbor message"};
      const ReputationChange ROUND_MESSAGE                = {100,   "Grandpa: Round message"};
      const ReputationChange BASIC_VALIDATED_CATCH_UP     = {200,   "Grandpa: Catch-up message"};
      const ReputationChange BASIC_VALIDATED_COMMIT       = {100,   "Grandpa: Commit"};
      const ReputationChange PER_EQUIVOCATION             = {10,    ""};
    }  // namespace benefit
    // clang-format on

  }  // namespace reputation

}  // namespace kagome::network

#endif  // KAGOME_NETWORK_REPUTATIONCHANGE
