/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STATE_API_IMPL_HPP
#define KAGOME_STATE_API_IMPL_HPP

#include "api/state/state_api.hpp"

namespace kagome::api {

  class StateApiImpl: public StateApi {
   public:
    StateApiImpl();

    outcome::result<common::Buffer> getStorage(
        const common::Buffer &key) override;
    outcome::result<common::Buffer> getStorage(
        const common::Buffer &key, const primitives::BlockHash &block) override;

   private:

  };

}  // namespace kagome::api

#endif  // KAGOME_STATE_API_IMPL_HPP
