/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_WRITABLE_HPP
#define KAGOME_WRITABLE_HPP

#include <system_error>
#include "common/buffer.hpp"

namespace libp2p::basic {
  class Writable {
   public:
    using ErrorCodeCallback = void(std::error_code, size_t);

    /**
     * @brief Asynchronously write {@param msg} and execute {@param handler}
     * when its done. Does not block.
     * @param msg message to be sent
     * @param handler callback that is executed when write finished
     */
    virtual void writeAsync(
        const kagome::common::Buffer &msg,
        std::function<ErrorCodeCallback> handler) noexcept = 0;

    /**
     * This function is used to write data. The function
     * call will block until one or more bytes of the data has been written
     * successfully, or until an error occurs.
     * @param msg to be written
     * @return error code if any error occurred.
     *
     * @note The write_some operation may not transmit all of the data to the
     * peer. Consider using the @ref write function if you need to ensure that
     * all data is written before the blocking operation completes.
     */
    virtual std::error_code writeSome(
        const kagome::common::Buffer &msg) noexcept = 0;

    /**
     * @brief This function is used to write data. The function
     * call will block until all bytes has been written successfully, or until
     * an error occurs.
     * @param msg to be written
     * @return error code if any error occurred.
     */
    virtual std::error_code write(
        const kagome::common::Buffer &msg) noexcept = 0;
  };
}  // namespace libp2p::basic

#endif  // KAGOME_WRITABLE_HPP
