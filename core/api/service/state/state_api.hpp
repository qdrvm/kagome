/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_API_STATE_API_HPP
#define KAGOME_API_STATE_API_HPP

#include <optional>

#include "common/buffer.hpp"
#include "outcome/outcome.hpp"
#include "primitives/common.hpp"
#include "primitives/version.hpp"

namespace kagome::api {

  class StateApi {
   public:
    virtual ~StateApi() = default;
    virtual outcome::result<common::Buffer> getStorage(
        const common::Buffer &key) const = 0;
    virtual outcome::result<common::Buffer> getStorage(
        const common::Buffer &key, const primitives::BlockHash &at) const = 0;
    /// Returns Version of the Runtime.
    virtual outcome::result<primitives::Version> getRuntimeVersion(
        std::optional<primitives::BlockHash> const &at) const = 0;
  };

}  // namespace kagome::api

#endif  // KAGOME_API_STATE_API_HPP
