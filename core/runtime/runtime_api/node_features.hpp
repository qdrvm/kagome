/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>
#include <optional>

#include <scale/bit_vector.hpp>

namespace kagome::runtime {
  struct NodeFeatures {
    /// A feature index used to identify a bit into the node_features array
    /// stored in the HostConfiguration.
    enum Index : uint8_t {
      /// Tells if tranch0 assignments could be sent in a single certificate.
      /// Reserved for:
      /// `<https://github.com/paritytech/polkadot-sdk/issues/628>`
      EnableAssignmentsV2 = 0,

      /// This feature enables the extension of
      /// `BackedCandidate::validator_indices` by 8 bits.
      /// The value stored there represents the assumed core index where the
      /// candidates are backed. This is needed for the elastic scaling MVP.
      ElasticScalingMVP = 1,

      /// Tells if the chunk mapping feature is enabled.
      /// Enables the implementation of
      /// [RFC-47](https://github.com/polkadot-fellows/RFCs/blob/main/text/0047-assignment-of-availability-chunks.md).
      /// Must not be enabled unless all validators and collators have stopped
      /// using `req_chunk` protocol version 1. If it is enabled, validators can
      /// start systematic chunk recovery.
      AvailabilityChunkMapping = 2,

      /// Enables node side support of `CoreIndex` committed candidate receipts.
      /// See [RFC-103](https://github.com/polkadot-fellows/RFCs/pull/103) for
      /// details.
      /// Only enable if at least 2/3 of nodes support the feature.
      CandidateReceiptV2 = 3,

      /// First unassigned feature bit.
      /// Every time a new feature flag is assigned it should take this value.
      /// and this should be incremented.
      FirstUnassigned = 4,
    };

    bool has(Index index) const {
      return bits and index < bits->size() and (*bits)[index];
    }

    std::optional<scale::BitVector> bits;
  };
}  // namespace kagome::runtime
