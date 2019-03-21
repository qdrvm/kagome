/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STREAM_HPP
#define KAGOME_STREAM_HPP

#include "common/result.hpp"
#include "libp2p/basic_interfaces/writable.hpp"
#include "libp2p/common/network_message.hpp"

namespace libp2p::stream {
  /**
   * Stream between two peers in the network
   */
  class Stream : public basic::Writable {
    /**
     * Read messages from the stream
     * @return observable to messages, received by that stream
     */
    // TODO(@warchant): review types, rewrite docs
    virtual std::vector<common::NetworkMessage> read() const = 0;
  };
}  // namespace libp2p::stream

#endif  // KAGOME_STREAM_HPP
