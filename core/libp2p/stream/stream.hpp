/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STREAM_HPP
#define KAGOME_STREAM_HPP

#include <rxcpp/rx-observable.hpp>
#include "common/result.hpp"
#include "libp2p/basic_interfaces/writable.hpp"
#include "libp2p/common_objects/network_message.hpp"

namespace libp2p {
  namespace stream {
    /**
     * Stream between two peers in the network
     */
    class Stream : public basic_interfaces::Writable {
      /**
       * Read messages from the stream
       * @return observable to messages, received by that stream
       */
      virtual rxcpp::observable<common::NetworkMessage> read() const = 0;
    };
  }  // namespace stream
}  // namespace libp2p

#endif  // KAGOME_STREAM_HPP
