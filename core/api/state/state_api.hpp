/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_API_STATE_STATE_API_HPP
#define KAGOME_API_STATE_STATE_API_HPP

#include "common/buffer.hpp"
#include "primitives/common.hpp"

namespace kagome::api {
  class StateApi {
   public:
    virtual ~StateApi() = default;

    /**
     * @brief Get a value from the storage by the provided \param key
     */
    virtual outcome::result<common::Buffer> getStorage(
        const common::Buffer &key) = 0;

    /**
     * @brief Get a value from the storage by the provided \param key at the
     * \param block's state
     */
    virtual outcome::result<common::Buffer> getStorage(
        const common::Buffer &key, const primitives::BlockHash &block) = 0;


  };

}  // namespace kagome::api

#endif  // KAGOME_API_STATE_STATE_API_HPP
