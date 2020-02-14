/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/state/impl/state_api_impl.hpp"

namespace kagome::api {

  outcome::result<common::Buffer> StateApiImpl::getStorage(
      const common::Buffer &key) {
    return outcome::result<common::Buffer>();
  }

  outcome::result<common::Buffer> StateApiImpl::getStorage(
      const common::Buffer &key, const primitives::BlockHash &at) {
    return outcome::result<common::Buffer>();
  }
}  // namespace kagome::api
