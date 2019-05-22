/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_RAW_CONNECTION_HPP
#define KAGOME_RAW_CONNECTION_HPP

#include "libp2p/basic/readwritecloser.hpp"

namespace libp2p::connection {

  struct RawConnection : public basic::ReadWriteCloser {
    ~RawConnection() override = default;

    /// returns if this side is an initiator of this connection, or false if it
    /// was a server in that case
    virtual bool isInitiator() const noexcept = 0;
  };

}  // namespace libp2p::connection

#endif  // KAGOME_RAW_CONNECTION_HPP
