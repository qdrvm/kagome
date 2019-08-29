/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_MUXER_ADAPTOR_HPP
#define KAGOME_MUXER_ADAPTOR_HPP

#include <memory>

#include <outcome/outcome.hpp>
#include "libp2p/basic/adaptor.hpp"
#include "libp2p/connection/capable_connection.hpp"
#include "libp2p/connection/secure_connection.hpp"

namespace libp2p::muxer {
  /**
   * Strategy to upgrade connections to muxed
   */
  struct MuxerAdaptor : public basic::Adaptor {
    using CapConnCallbackFunc = std::function<void(
        outcome::result<std::shared_ptr<connection::CapableConnection>>)>;

    ~MuxerAdaptor() override = default;

    /**
     * Make a muxed connection from the secure one, using this adaptor
     * @param conn - connection to be upgraded
     * @param cb - callback with an upgraded connection or error
     */
    virtual void muxConnection(
        std::shared_ptr<connection::SecureConnection> conn,
        CapConnCallbackFunc cb) const = 0;
  };
}  // namespace libp2p::muxer

#endif  // KAGOME_MUXER_ADAPTOR_HPP
