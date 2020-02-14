/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_API_STATE_API_HPP
#define KAGOME_API_STATE_API_HPP

#include "common/buffer.hpp"
#include "outcome/outcome.hpp"

namespace kagome::api {

  class StateApi {
   public:
    outcome::result<common::Buffer> getStorage(const common::Buffer &key);
    outcome::result<common::Buffer> getStorage(const common::Buffer &key,
                                               const primitives::BlockHash &at);
  };

}  // namespace kagome::api

#endif  // KAGOME_API_STATE_API_HPP
