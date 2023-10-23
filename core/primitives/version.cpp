/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/version.hpp"

#include <algorithm>

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
