/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_READER_HPP
#define KAGOME_READER_HPP

#include <vector>

#include <gsl/span>
#include <outcome/outcome.hpp>

namespace libp2p::basic {

  struct Reader {
    virtual ~Reader() = default;

    /**
     * @brief Blocks until all {@param bytes} bytes are read.
     * @param bytes number of bytes to read
     * @return vector of bytes or error code
     */
    virtual outcome::result<std::vector<uint8_t>> read(size_t bytes) = 0;

    /**
     * @brief Blocks until any number (not more that {@param bytes}) of bytes
     * are read.
     * @param bytes number of bytes to read
     * @return vector of bytes or error code
     */
    virtual outcome::result<std::vector<uint8_t>> readSome(size_t bytes) = 0;

    virtual outcome::result<size_t> read(gsl::span<uint8_t> buf) = 0;

    virtual outcome::result<size_t> readSome(gsl::span<uint8_t> buf) = 0;
  };

}  // namespace libp2p::basic

#endif  // KAGOME_READER_HPP
