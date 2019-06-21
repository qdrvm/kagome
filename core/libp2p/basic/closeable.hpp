/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CLOSEABLE_HPP
#define KAGOME_CLOSEABLE_HPP

#include <functional>

#include <outcome/outcome.hpp>

namespace libp2p::basic {

  class Closeable {
   public:
    using CloseCallback = void(outcome::result<void>);
    using CloseCallbackFunc = std::function<CloseCallback>;

    virtual ~Closeable() = default;

    /**
     * @brief Function that is used to check if current object is closed.
     * @return true if closed, false otherwise
     */
    virtual bool isClosed() const = 0;

    /**
     * @brief Closes current object
     * @param cb - callback, which is called after the object is closed, or
     * error happens
     */
    virtual void close(CloseCallbackFunc cb) = 0;
  };

}  // namespace libp2p::basic

#endif  // KAGOME_CLOSEABLE_HPP
