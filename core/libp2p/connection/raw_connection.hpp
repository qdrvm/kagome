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
  };

}  // namespace libp2p::connection

#endif  // KAGOME_RAW_CONNECTION_HPP
