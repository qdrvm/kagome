/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_READER_HPP
#define KAGOME_READER_HPP

#include <boost/system/error_code.hpp>
#include <gsl/span>

namespace libp2p::basic {

  struct Reader {
    using ErrorCode = boost::system::error_code;
    using ReadCallback = void(const ErrorCode & /*ec*/, size_t /*read bytes */);
    using ReadCallbackFunc = std::function<ReadCallback>;

    virtual ~Reader() = default;

    /**
     * @brief Reads exactly {@code} out.size() {@nocode} bytes to the buffer.
     * @param out output argument. Read data will be written to this buffer.
     * @param cb callback with result of operation
     */
    virtual void read(gsl::span<uint8_t> out, ReadCallbackFunc cb) = 0;

    /**
     * @brief Reads up to {@code} out.size() {@nocode} bytes to the buffer.
     * @param out output argument. Read data will be written to this buffer.
     * @param cb callback with result of operation
     */
    virtual void readSome(gsl::span<uint8_t> out, ReadCallbackFunc cb) = 0;
  };

}  // namespace libp2p::basic

#endif  // KAGOME_READER_HPP
