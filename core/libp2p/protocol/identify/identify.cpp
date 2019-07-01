/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string_view>

#include "libp2p/protocol/identify/identify.hpp"

namespace {
  constexpr std::string_view kIdentifyProto = "/ipfs/id/1.0.0";
}

namespace libp2p::protocol {
  Identify::Identify(HostSPtr host, kagome::common::Logger log)
      : host_{std::move(host)}, log_{std::move(log)} {
    host_->setProtocolHandler(kIdentifyProto, identify);
  }

  void Identify::identify(std::shared_ptr<connection::Stream> stream) {}
}  // namespace libp2p::protocol
