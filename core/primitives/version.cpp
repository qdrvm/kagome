/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/version.hpp"

#include <algorithm>

#include "crypto/hasher/hasher_impl.hpp"

namespace {
  static std::unique_ptr<kagome::crypto::Hasher> kHasher =
      std::make_unique<kagome::crypto::HasherImpl>();
}

namespace kagome::primitives::detail {

  std::optional<uint32_t> coreVersionFromApis(const ApisVec &apis) {
    auto api_id = kHasher->blake2b_64(common::Buffer::fromString("Core"));
    auto result = std::find_if(apis.begin(), apis.end(), [&api_id](auto &api) {
      if (api.first == api_id) {
        return true;
      }
      return false;
    });
    if (result == apis.end()) {
      return std::nullopt;
    }
    return result->second;
  }

}  // namespace kagome::primitives::detail
