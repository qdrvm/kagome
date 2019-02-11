/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef UGUISU_MUXED_CONNECTION_HPP
#define UGUISU_MUXED_CONNECTION_HPP

#include "common/result.hpp"
#include "libp2p/connection/connection.hpp"
#include "libp2p/stream/stream.hpp"

namespace libp2p {
  namespace connection {
    /**
     * Connection complement, which appears, when it gets muxed
     */
    class MuxedConnection : public Connection {
      /**
       * Create a new stream over this muxed connection
       * @return a created stream in case of success, error otherwise
       */
      virtual uguisu::expected::Result<std::unique_ptr<stream::Stream>,
                                       std::string>
      newStream() = 0;
    };
  }  // namespace connection
}  // namespace libp2p

#endif  // UGUISU_MUXED_CONNECTION_HPP
