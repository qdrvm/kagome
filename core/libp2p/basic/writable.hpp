/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_WRITABLE_HPP
#define KAGOME_WRITABLE_HPP

#include <functional>

#include <boost/asio/buffer.hpp>
#include <boost/asio/streambuf.hpp>

namespace libp2p::basic {
  class Writable {
   public:
    using CompletionHandler = void(const std::error_code & /* ec*/,
                                   size_t /* written */);

    virtual ~Writable() = default;

    /**
     * @brief Asynchronously write buffer. Once operation completed, completion
     * handler {@param cb} is executed.
     *
     * @see boost::asio::buffer to pass any buffer as first argument
     *
     * @param buf buffer to write.
     * @param cb completion hander that is executed after operation succeeds.
     */
    virtual void asyncWrite(const boost::asio::const_buffer &buf,
                            std::function<CompletionHandler> cb)  = 0;

    virtual void asyncWrite(boost::asio::streambuf &buf,
                            std::function<CompletionHandler> cb)  = 0;
  };
}  // namespace libp2p::basic

#endif  // KAGOME_WRITABLE_HPP
