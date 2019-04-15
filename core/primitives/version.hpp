/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_PRIMITIVES_VERSION_HPP
#define KAGOME_CORE_PRIMITIVES_VERSION_HPP

#include <array>
#include <cstdint>
#include <vector>

namespace kagome::primitives {

  /**
   * This is the same structure as RuntimeVersion from substrate
   * https://github.com/paritytech/substrate/blob/master/core/sr-version/src/lib.rs
   */

  /// The identity of a particular API interface that the runtime might provide.
  using ApiId = std::array<uint8_t, 8>;

  using Api = std::pair<ApiId, uint32_t>;

  /// A vector of pairs of `ApiId` and a `u32` for version.
  using ApisVec = std::vector<Api>;

  /**
   * Runtime version.
   * This should not be thought of as classic Semver (major/minor/tiny).
   * This triplet have different semantics and mis-interpretation could cause
   * problems. In particular: bug fixes should result in an increment of
   * `spec_version` and possibly `authoring_version`, absolutely not
   * `impl_version` since they change the semantics of the runtime.
   */
  class Version {
   public:
    Version(std::string spec_name, std::string impl_name,
            uint32_t authoring_version, uint32_t impl_version,
            const ApisVec &apis);

    Version() = delete;

    /**
     * Identifies the different Substrate runtimes. There'll be at least
     * polkadot and node.
     */
    const std::string &specName();

    /**
     * Name of the implementation of the spec. This is of little consequence
     * for the node and serves only to differentiate code of different
     * implementation teams. For this codebase, it will be kagome. If there
     * were a non-Rust implementation of the Polkadot runtime (e.g. C++), then
     * it would identify itself with an accordingly different impl_name.
     */
    const std::string &implName();

    /**
     * authoring_version is the version of the authorship interface
     */
    uint32_t authoringVersion();

    /**
     * Version of the implementation of the specification. Nodes are free to
     * ignore this; it serves only as an indication that the code is different;
     * as long as the other two versions are the same then while the actual
     * code may be different, it is nonetheless required to do the same thing.
     * Non-consensus-breaking optimizations are about the only changes that
     * could be made which would result in only the impl_version changing.
     */
    uint32_t implVersion();

    /// List of supported API "features" along with their versions.
    const std::vector<std::string> &apis();

   private:
    std::string spec_name_;
    std::string impl_name_;
    uint32_t authoring_version_;
    uint32_t impl_version_;
    ApisVec apis_;
  };
}  // namespace kagome::primitives

#endif  // KAGOME_CORE_PRIMITIVES_VERSION_HPP
