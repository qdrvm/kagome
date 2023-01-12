/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_RUNTIME_DISPATCH_INFO_HPP
#define KAGOME_RUNTIME_DISPATCH_INFO_HPP

namespace kagome::primitives {

  using OldWeight = uint64_t;

  struct Weight {
    SCALE_TIE(2);
    Weight() = default;

    explicit Weight(OldWeight w) : ref_time{w}, proof_size{0} {}

    Weight(uint64_t ref_time, uint64_t proof_size)
        : ref_time{ref_time}, proof_size{proof_size} {}

    // The weight of computational time used based on some reference hardware.
    scale::CompactInteger ref_time;
    // The weight of storage space used by proof of validity.
    scale::CompactInteger proof_size;
  };

  enum class DispatchClass {
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

  /** Information related to a dispatchable class, weight, and fee that can be
   * queried from the runtime.
   */
  template <typename Weight>
  struct RuntimeDispatchInfo {
    using Balance = uint32_t;

    Weight weight;
    DispatchClass dispatch_class;

    /** The inclusion fee of this dispatch. This does not include a tip or
     * anything else that depends on the signature (i.e. depends on a
     * `SignedExtension`).
     */
    Balance partial_fee;
  };

  template <class Stream,
            typename Weight,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const RuntimeDispatchInfo<Weight> &v) {
    return s << v.weight << v.dispatch_class << v.partial_fee;
  }

  template <class Stream,
            typename Weight,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, RuntimeDispatchInfo<Weight> &v) {
    uint8_t dispatch_class;
    s >> v.weight >> dispatch_class >> v.partial_fee;
    v.dispatch_class = static_cast<DispatchClass>(dispatch_class);
    return s;
  }

}  // namespace kagome::primitives

#endif  // KAGOME_RUNTIME_DISPATCH_INFO_HPP
