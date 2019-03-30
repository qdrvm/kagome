/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CLOSEABLE_HPP
#define KAGOME_CLOSEABLE_HPP

#include <system_error>

namespace libp2p::basic {

  class Closeable {
   public:

    /**
     * @brief Function that is used to check if current object is closed.
     * @return true if closed, false otherwise
     */
    virtual bool isClosed() const = 0;

    /**
     * @brief Closes current object.
     * @return error code if anything went wrong
     */
    virtual std::error_code close() = 0;
  };

}  // namespace libp2p::basic

#endif  // KAGOME_CLOSEABLE_HPP
