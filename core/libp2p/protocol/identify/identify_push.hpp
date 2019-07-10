/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_IDENTIFY_PUSH_HPP
#define KAGOME_IDENTIFY_PUSH_HPP

#include <memory>

#include "libp2p/protocol/base_protocol.hpp"
#include "libp2p/protocol/identify/identify.hpp"

namespace libp2p::protocol {
  /**
   * Implementation of Identify-Push protocol, which is used to inform known
   * peers about changes in this peer's configuration by sending or receiving a
   * whole Identify message. Read more:
   * https://github.com/libp2p/specs/blob/master/identify/README.md
   */
  class IdentifyPush : public BaseProtocol {
   public:
    explicit IdentifyPush(std::shared_ptr<Identify> id);

    peer::Protocol getProtocolId() const override;

    /**
     * In Identify-Push, handle means we accepted an Identify-Push stream and
     * should receive an Identify message
     */
    void handle(StreamResult stream_res) override;

    /**
     * Start this Identify-Push, so that it subscribes to events, when some
     * basic info of our peer changes
     */
    void start();

   private:
    std::shared_ptr<Identify> id_;
  };
}  // namespace libp2p::protocol

#endif  // KAGOME_IDENTIFY_PUSH_HPP
