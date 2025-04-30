/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <optional>
#include <string>
#include <vector>

#include "common/blob.hpp"
#include "common/buffer.hpp"
#include "crypto/hasher/hasher_impl.hpp"
#include "scale/kagome_scale.hpp"

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

  namespace detail {
    inline std::optional<uint32_t> coreVersionFromApis(const ApisVec &apis) {
      // We break DI principle here since we need to use hasher in decode scale
      // operator overload and we cannot inject it there
      static auto api_id =
          crypto::HasherImpl{}.blake2b_64(common::Buffer::fromString("Core"));

      auto result = std::ranges::find_if(
          apis, [&](auto &api) { return api.first == api_id; });
      if (result == apis.end()) {
        return std::nullopt;
      }
      return result->second;
    }
  }  // namespace detail

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

    uint32_t transaction_version = 1u;

    /// Version of the state implementation used by this runtime.
    /// This field is kept for backward compatibility. It always has the same
    /// value as system_version.
    uint8_t state_version = 0u;

    /// Version of the system implementation used by this runtime.
    /// Originally named state_version, but renamed to system_version in newer
    /// versions. Both fields are maintained with identical values for
    /// compatibility.
    uint8_t system_version = 0u;

    bool operator==(const Version &rhs) const = default;

    /**
     * `Decode` while giving a "version hint"
     * There exists multiple versions of [`RuntimeVersion`] and they are
     * versioned using the `Core` runtime api:
     * - `Core` version < 3 is a runtime version without a transaction version
     * and state version.
     * - `Core` version 3 is a runtime version without a state version.
     * - `Core` version 4 is the latest runtime version.
     * `core_version` hint is used by `readEmbeddedVersion`, because
     * `Version.apis` is stored separately from other `Version` fields.
     * https://github.com/paritytech/polkadot-sdk/blob/aaf0443591b134a0da217d575161872796e75059/substrate/primitives/version/src/lib.rs#L242
     */
    static outcome::result<Version> decodeConsideringToCoreVersion(
        scale::Decoder &decoder, std::optional<uint32_t> core_version) {
      Version v;
      try {
        decode(std::tie(v.spec_name,
                        v.impl_name,
                        v.authoring_version,
                        v.spec_version,
                        v.impl_version,
                        v.apis),
               decoder);
      } catch (std::system_error &e) {
        return outcome::failure(e.code());
      }

      if (not core_version) {
        core_version = detail::coreVersionFromApis(v.apis);
      }
      // old Kusama runtimes do not contain transaction_version and
      // state_version
      // https://github.com/paritytech/substrate/blob/1b3ddae9dec6e7653b5d6ef0179df1af831f46f0/primitives/version/src/lib.rs#L238
      if (core_version and *core_version >= 3) {
        try {
          decode(v.transaction_version, decoder);
        } catch (std::system_error &e) {
          return outcome::failure(e.code());
        }
      } else {
        v.transaction_version = 1;
      }
      if (core_version and *core_version >= 4) {
        try {
          decode(v.system_version, decoder);
          v.state_version = v.system_version;
        } catch (std::system_error &e) {
          return outcome::failure(e.code());
        }
      } else {
        v.system_version = 0;
        v.state_version = 0;
      }
      return v;
    }

    friend void decode(Version &v, scale::Decoder &decoder) {
      // `.value()` may throw, `scale::decode` will catch that
      v = Version::decodeConsideringToCoreVersion(decoder, std::nullopt)
              .value();
    }
  };

}  // namespace kagome::primitives
