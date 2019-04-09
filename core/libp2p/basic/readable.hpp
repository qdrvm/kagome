/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_READABLE_HPP
#define KAGOME_READABLE_HPP

#include <functional>
#include <system_error>

#include <boost/asio/buffer.hpp>
#include <boost/asio/streambuf.hpp>

namespace libp2p::basic {

  class Readable {
   public:
    //
    using CompletionHandler = void(const std::error_code & /* error */,
                                   size_t /* read_bytes */);

    /**
     * @brief Asynchronously read exactly {@param to_read} bytes into {@param
     * mut} buffer. Once operation succeeds, completion handler {@param cb} is
     * executed.
     *
     * This function reads at most {@code} min(to_read, mut.size()); {@endcode}
     * bytes.
     *
     * @see boost::asio::buffer to pass any buffer as first argument
     *
     * @param mut output buffer. Caller MUST ensure buffer remains valid after
     * the call. Buffer must be of size {@param to_read} or bigger to read
     * {@param to_read} bytes.
     * @param to_read number of bytes to read.
     * @param cb completion handler that is executed when operation completes.
     * First argument is error code, second is number of bytes read from the
     * socket.
     */
    virtual void asyncRead(boost::asio::mutable_buffer &mut, uint32_t to_read,
                           std::function<CompletionHandler> cb) noexcept = 0;

    /// with this you can write asyncRead(boost::asio::buffer(rdbuf->toVector(),
    /// kSize), ...)
    virtual void asyncRead(boost::asio::mutable_buffer &&mut, uint32_t to_read,
                           std::function<CompletionHandler> cb) noexcept = 0;

    virtual void asyncRead(boost::asio::streambuf &streambuf, uint32_t to_read,
                           std::function<CompletionHandler> cb) noexcept = 0;
  };

}  // namespace libp2p::basic

#endif  // KAGOME_READABLE_HPP
