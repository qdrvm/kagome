/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_READABLE_HPP
#define KAGOME_READABLE_HPP

#include "libp2p/common/network_message.hpp"

namespace libp2p::basic {

  class Readable {
    /**
     * Read message from the connection
     * @return optional to message, received by that connection
     */
    virtual boost::optional<common::NetworkMessage> read() const = 0;
  };

}  // namespace libp2p::basic

#endif  // KAGOME_READABLE_HPP
