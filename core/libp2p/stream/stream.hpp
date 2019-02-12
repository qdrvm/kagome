/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STREAM_HPP
#define KAGOME_STREAM_HPP

#include <rxcpp/rx-observable.hpp>
#include "common/result.hpp"
#include "libp2p/common_objects/network_message.hpp"

namespace libp2p {
  namespace stream {
    /**
     * Stream between two peers in the network
     */
    class Stream {
      /**
       * Write message to the stream
       * @param msg to be written
       * @return void in case of success, error otherwise
       */
      virtual rxcpp::observable<kagome::expected::Result<void, std::string>>
      write(const common::NetworkMessage &msg) const = 0;

      /**
       * Read messages from the stream
       * @return observable to messages, received by that stream, in case of
       * success, error otherwise
       */
      virtual kagome::expected::
          Result<rxcpp::observable<common::NetworkMessage>, std::string>
          read() const = 0;
    };
  }  // namespace stream
}  // namespace libp2p

#endif  // KAGOME_STREAM_HPP
