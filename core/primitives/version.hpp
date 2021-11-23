/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_PRIMITIVES_VERSION_HPP
#define KAGOME_CORE_PRIMITIVES_VERSION_HPP

#include <array>
#include <string>
#include <vector>

#include "common/blob.hpp"

namespace kagome::primitives {
  /**
   * This is the same structure as RuntimeVersion from substrate
   * https://github.com/paritytech/substrate/blob/e670c040d42132da982e156e25a385df213354ed/primitives/version/src/lib.rs
   */

  /**
   * @brief The identity of a particular API interface that the runtime might
   * provide.
   */
  using ApiId = common::Blob<8u>;

  /**
   * @brief single Api item
   */
  using Api = std::pair<ApiId, uint32_t>;

  /**
   * @brief A vector of pairs of `ApiId` and a `u32` for version.
   */
  using ApisVec = std::vector<Api>;

  /**
   * Runtime version.
   * This should not be thought of as classic Semver (major/minor/tiny).
   * This triplet have different semantics and mis-interpretation could cause
   * problems. In particular: bug fixes should result in an increment of
   * `spec_version` and possibly `authoring_version`, absolutely not
   * `impl_version` since they change the semantics of the runtime.
   */
  struct Version {
    /**
     * Identifies the different Substrate runtimes. There'll be at least
     * polkadot and node.
     */
    std::string spec_name;

    /**
     * Name of the implementation of the spec. This is of little consequence
     * for the node and serves only to differentiate code of different
     * implementation teams. For this codebase, it will be kagome. If there were
     * a non-Rust implementation of the Polkadot runtime (e.g. C++), then it
     * would identify itself with an accordingly different impl_name.
     * */
    std::string impl_name;

    /// authoring_version is the version of the authorship interface
    uint32_t authoring_version = 0u;

    /**
     * Version of the implementation of the specification. Nodes are free to
     * ignore this; it serves only as an indication that the code is different;
     * as long as the other two versions are the same then while the actual
     * code may be different, it is nonetheless required to do the same thing.
     * Non-consensus-breaking optimizations are about the only changes that
     * could be made which would result in only the impl_version changing.
     */
    uint32_t spec_version = 0u;

    uint32_t impl_version = 0u;

    /// List of supported API "features" along with their versions.
    ApisVec apis;

    uint32_t transaction_version = 0u;

    bool operator==(const Version &rhs) const {
      return spec_name == rhs.spec_name and impl_name == rhs.impl_name
             and authoring_version == rhs.authoring_version
             and impl_version == rhs.impl_version and apis == rhs.apis
             and spec_version == rhs.spec_version
             and transaction_version == rhs.transaction_version;
    }

    bool operator!=(const Version &rhs) const {
      return !operator==(rhs);
    }
  };

  /**
   * @brief outputs object of type Version to stream
   * @tparam Stream output stream type
   * @param s stream reference
   * @param v value to output
   * @return reference to stream
   */
  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const Version &v) {
    return s << v.spec_name << v.impl_name << v.authoring_version
             << v.spec_version << v.impl_version << v.apis
             << v.transaction_version;
  }

  /**
   * @brief decodes object of type Version from stream
   * @tparam Stream input stream type
   * @param s stream reference
   * @param v value to decode
   * @return reference to stream
   */
  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, Version &v) {
    s >> v.spec_name >> v.impl_name >> v.authoring_version >> v.spec_version
        >> v.impl_version >> v.apis;
    if (s.hasMore(sizeof(v.transaction_version)))
      s >> v.transaction_version;
    return s;
  }
}  // namespace kagome::primitives

#endif  // KAGOME_CORE_PRIMITIVES_VERSION_HPP
