/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/state/impl/state_api_impl.hpp"

namespace kagome::api {

  outcome::result<common::Buffer> StateApiImpl::getStorage(
      const kagome::common::Buffer &key) {
    return getStorage();
  }

  outcome::result<common::Buffer> StateApiImpl::getStorage(
      const kagome::common::Buffer &key,
      const kagome::primitives::BlockHash &block) {
    ;
  }

}  // namespace kagome::api
