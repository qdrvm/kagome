/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/protocol/identify/identify_push.hpp"

#include <string>

namespace {
  const std::string kIdentifyPushProtocol = "/ipfs/id/push/1.0.0";
}

namespace libp2p::protocol {
  IdentifyPush::IdentifyPush(std::shared_ptr<Identify> id)
      : id_{std::move(id)} {}

  peer::Protocol IdentifyPush::getProtocolId() const {
    return kIdentifyPushProtocol;
  }

  void IdentifyPush::handle(StreamResult stream_res) {
    if (!stream_res) {
      return;
    }
    id_->receiveIdentify(std::move(stream_res.value()));
  }

  void IdentifyPush::start() {

  }
}  // namespace libp2p::protocol
