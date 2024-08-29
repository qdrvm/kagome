/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <ec-cpp/ec-cpp.hpp>

#include "parachain/availability/chunks.hpp"
#include "runtime/runtime_api/parachain_host.hpp"

namespace kagome::parachain {

  /// Obtain the threshold of systematic chunks that should be enough to recover
  /// the data.
  ///
  /// If the regular `recovery_threshold` is a power of two, then it returns the
  /// same value. Otherwise, it returns the next lower power of two.
  ///
  /// https://github.com/paritytech/polkadot-sdk/blob/d2fd53645654d3b8e12cbf735b67b93078d70113/polkadot/erasure-coding/src/lib.rs#L120
  inline outcome::result<ChunkIndex> systematic_recovery_threshold(
      size_t n_validators) {
    auto threshold_res = ec_cpp::getRecoveryThreshold(n_validators);
    [[unlikely]] if (ec_cpp::resultHasError(threshold_res)) {
      return kagome::ErasureCodingError(kagome::toErasureCodingError(
          ec_cpp::resultGetError(std::move(threshold_res))));
    }
    auto threshold = ec_cpp::resultGetValue(std::move(threshold_res));

    // if the threshold is a power of 2, return it
    if ((threshold & (threshold - 1)) == 0) {
      return threshold;
    }

    // tick down to the next lower power of 2
    ChunkIndex threshold_po2 = 1;
    while (threshold_po2 < threshold) {
      threshold_po2 <<= 1;
    }

    return threshold_po2;
  }

  inline bool availability_chunk_mapping_is_enabled(
      std::optional<runtime::ParachainHost::NodeFeatures> node_features) {
    // none if node_features is not defined
    [[unlikely]] if (not node_features.has_value()) { return false; }

    const auto &features = node_features.value();

    static const auto feature =
        runtime::ParachainHost::NodeFeatureIndex::AvailabilityChunkMapping;

    // none if needed feature is out of bound
    [[unlikely]] if (feature >= features.bits.size()) { return false; }

    return features.bits[feature];
  }

  /// Compute the per-validator availability chunk index.
  /// WARNING: THIS FUNCTION IS CRITICAL TO PARACHAIN CONSENSUS.
  /// Any modification to the output of the function needs to be coordinated via
  /// the runtime. It's best to use minimal/no external dependencies.
  outcome::result<ChunkIndex> availability_chunk_index(
      std::optional<runtime::ParachainHost::NodeFeatures> node_features,
      size_t n_validators,
      CoreIndex core_index,
      ValidatorIndex validator_index) {
    if (availability_chunk_mapping_is_enabled(node_features)) {
      OUTCOME_TRY(systematic_threshold,
                  systematic_recovery_threshold(n_validators));

      auto core_start_pos = core_index * systematic_threshold;

      return ChunkIndex((core_start_pos + validator_index) % n_validators);
    }
    return validator_index;
  }
};  // namespace kagome::parachain
