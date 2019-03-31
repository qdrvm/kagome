/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_READABLE_HPP
#define KAGOME_READABLE_HPP

#include <functional>
#include <system_error>

#include <outcome/outcome.hpp>
#include "common/buffer.hpp"

namespace libp2p::basic {

  class Readable {
   public:
    using BufferResult = outcome::result<kagome::common::Buffer>;
    using BufferResultCallback = void(BufferResult);

    /**
     * @brief This function is used to read data. The function call will block
     * until exactly {@param to_read} bytes of data has been read successfully,
     * or until an error occurs.
     * @param to_read number of bytes to read.
     * @return result of the operation.
     */
    virtual BufferResult read(uint32_t to_read) = 0;

    /**
     * @brief This function is used to read data. The function call will block
     * until one or more bytes of data has been read successfully, or until an
     * error occurs.
     * @param to_read number of bytes to read.
     * @return result of the operation.
     */
    virtual BufferResult readSome(uint32_t to_read) = 0;

    /**
     * @brief Asynchronously read everything that was sent to our socket.
     * @param cb Callback that accepts read result.
     */
    virtual void readAsync(std::function<BufferResultCallback> cb) noexcept = 0;
  };

}  // namespace libp2p::basic

#endif  // KAGOME_READABLE_HPP
