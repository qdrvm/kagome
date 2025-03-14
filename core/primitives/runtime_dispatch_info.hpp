/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <scale/scale.hpp>

#include "common/unused.hpp"

namespace kagome::primitives {

  // obsolete weight format used in TransactionPayment API versions less than 2
  using OldWeight = scale::Compact<uint64_t>;

  struct Weight {
    // The weight of computational time used based on some reference hardware.
    scale::Compact<uint64_t> ref_time;
    // The weight of storage space used by proof of validity.
    scale::Compact<uint64_t> proof_size{0};

    bool operator==(const Weight &) const = default;
  };

  // for some reason encoded as variant in substrate, thus custom encode/decode
  // operators
  enum class DispatchClass : uint8_t {
    Normal,
    Operational,
    /* A mandatory dispatch. These kinds of dispatch are always included
     * regardless of their weight, therefore it is critical that they are
     * separately validated to ensure that a malicious validator cannot craft
     * a valid but impossibly heavy block. Usually this just means ensuring
     * that the extrinsic can only be included once and that it is always very
     * light.
     *
     * Do *NOT* use it for extrinsics that can be heavy.
     *
     * The only real use case for this is inherent extrinsics that are
     * required to execute in a block for the block to be valid, and it solves
     * the issue in the case that the block initialization is sufficiently
     * heavy to mean that those inherents do not fit into the block.
     * Essentially, we assume that in these exceptional circumstances, it is
     * better to allow an overweight block to be created than to not allow any
     * block at all to be created.
     */
    Mandatory
  };

  using Balance = qtils::Tagged<scale::uint128_t, struct BalanceTag>;

  /** Information related to a dispatchable class, weight, and fee that can be
   * queried from the runtime.
   */
  template <typename Weight>
  struct RuntimeDispatchInfo {
    Weight weight;
    DispatchClass dispatch_class;

    /** The inclusion fee of this dispatch. This does not include a tip or
     * anything else that depends on the signature (i.e. depends on a
     * `SignedExtension`).
     */
    Balance partial_fee;
    bool operator==(const RuntimeDispatchInfo &) const = default;
  };

}  // namespace kagome::primitives
