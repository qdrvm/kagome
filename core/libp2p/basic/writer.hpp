/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_WRITER_HPP
#define KAGOME_WRITER_HPP

#include <gsl/span>
#include <outcome/outcome.hpp>

namespace libp2p::basic {

  struct Writer {
    virtual ~Writer() = default;

    /**
     * @brief Blocks untill all data from {@param in} has been written.
     * @return number of written bytes or error code
     */
    virtual outcome::result<size_t> write(gsl::span<const uint8_t> in) = 0;

    /**
     * @brief Blocks until SOME data from {@param in} has been written.
     * @return number of written bytes or error code
     */
    virtual outcome::result<size_t> writeSome(gsl::span<const uint8_t> in) = 0;
  };

}  // namespace libp2p::basic

#endif  // KAGOME_WRITER_HPP
