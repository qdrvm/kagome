/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/version.hpp"

#include <algorithm>

#include "common/buffer.hpp"
#include "crypto/hasher/hasher_impl.hpp"

namespace {

  // We break DI principle here since we need to use hasher in decode scale
  // operator overload and we cannot inject it there
  static std::unique_ptr<kagome::crypto::Hasher> kHasher =
      std::make_unique<kagome::crypto::HasherImpl>();
}  // namespace

namespace kagome::primitives::detail {

  std::optional<uint32_t> coreVersionFromApis(const ApisVec &apis) {
    auto result = std::find_if(apis.begin(), apis.end(), [](auto &api) {
      static auto api_id =
          kHasher->blake2b_64(common::Buffer::fromString("Core"));
      return api.first == api_id;
    });
    if (result == apis.end()) {
      return std::nullopt;
    }
    return result->second;
  }

}  // namespace kagome::primitives::detail

namespace kagome::primitives {
  outcome::result<Version> Version::decode(
      scale::ScaleDecoderStream &s, std::optional<uint32_t> core_version) {
    Version v;
    auto t = std::tie(v.spec_name,
                      v.impl_name,
                      v.authoring_version,
                      v.spec_version,
                      v.impl_version,
                      v.apis);
    OUTCOME_TRY(scale::decode(s, t));

    if (not core_version) {
      core_version = detail::coreVersionFromApis(v.apis);
    }
    // old Kusama runtimes do not contain transaction_version and
    // state_version
    // https://github.com/paritytech/substrate/blob/1b3ddae9dec6e7653b5d6ef0179df1af831f46f0/primitives/version/src/lib.rs#L238
    if (core_version and *core_version >= 3) {
      OUTCOME_TRY(scale::decode(s, v.transaction_version));
    } else {
      v.transaction_version = 1;
    }
    if (core_version and *core_version >= 4) {
      OUTCOME_TRY(scale::decode(s, v.state_version));
    } else {
      v.state_version = 0;
    }
    return v;
  }
}  // namespace kagome::primitives
