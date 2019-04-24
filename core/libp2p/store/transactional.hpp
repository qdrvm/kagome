/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TRANSACTIONAL_HPP
#define KAGOME_TRANSACTIONAL_HPP

#include <outcome/outcome.hpp>

namespace libp2p::store {

  /**
   * @brief Transactional interface for a database
   */
  class Transactional {
   public:
    virtual ~Transactional() = default;

    /**
     * @brief Starts transaction to underlying datastore.
     */
    virtual void startTransaction() = 0;

    /**
     * @brief Commits transaction data to datastore.
     * @return error code if unsuccessful.
     */
    virtual outcome::result<void> commit() = 0;
  };

}  // namespace libp2p::store

#endif  // KAGOME_TRANSACTIONAL_HPP
