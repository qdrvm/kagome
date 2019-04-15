/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_PRIMITIVES_VERSION_HPP_
#define KAGOME_CORE_PRIMITIVES_VERSION_HPP_

#include <array>
#include <cstdint>
#include <vector>

namespace kagome::primitives {

  /// The identity of a particular API interface that the runtime might provide.
  using ApiId = std::array<uint8_t, 8>;

  using Api = std::pair<ApiId, uint32_t>;

  /// A vector of pairs of `ApiId` and a `u32` for version. For `"std"` builds,
  /// this
  /// is a `Cow`.
  using ApisVec = std::vector<Api>;

  class Version {
   public:
    /// Identifies the different Substrate runtimes. There'll be at least
    /// polkadot and node. A different on-chain spec_name to that of the native
    /// runtime would normally result in node not attempting to sync or author
    /// blocks.
    std::string spec_name();

    /// Name of the implementation of the spec. This is of little consequence
    /// for the node and serves only to differentiate code of different
    /// implementation teams. For this codebase, it will be kagome. If there
    /// were a non-Rust implementation of the Polkadot runtime (e.g. C++), then
    /// it would identify itself with an accordingly different impl_name.
    std::string impl_name();

    /// authoring_version is the version of the authorship interface. An
    /// authoring node will not attempt to author blocks unless this is equal to
    /// its native runtime.
    uint32_t authoring_version();

    /// Version of the implementation of the specification. Nodes are free to
    /// ignore this; it serves only as an indication that the code is different;
    /// as long as the other two versions are the same then while the actual
    /// code may be different, it is nonetheless required to do the same thing.
    /// Non-consensus-breaking optimizations are about the only changes that
    /// could be made which would result in only the impl_version changing.
    uint32_t impl_version();

    /// List of supported API "features" along with their versions.
    std::vector<std::string> apis();
  };
}  // namespace kagome::primitives

#endif  // KAGOME_CORE_PRIMITIVES_VERSION_HPP_
