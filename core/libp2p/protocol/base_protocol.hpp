/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BASE_PROTOCOL_HPP
#define KAGOME_BASE_PROTOCOL_HPP

#include <outcome/outcome.hpp>
#include "libp2p/basic/adaptor.hpp"
#include "libp2p/connection/stream.hpp"

namespace libp2p::protocol {

  /**
   * @brief Base class for user-defined protocols.
   *
   * @example
   * {@code}
   * struct EchoProtocol: public BaseProtocol {
   *   ...
   * };
   *
   * std::shared_ptr<Network> nw = std::make_shared<NetworkImpl>(...);
   * std::shared_ptr<BaseProtocol> p = std::make_shared<EchoProtocol>();
   *
   * // register protocol handler (server side callback will be executed
   * // when client opens a stream to us)
   * nw->addProtocol(p);
   * {@nocode}
   */
  struct BaseProtocol : public basic::Adaptor {
    ~BaseProtocol() override = default;

    using StreamResult = outcome::result<std::shared_ptr<connection::Stream>>;

    /**
     * @brief Handler that is executed on responder (server) side of the
     * protocol.
     * @param cb callback that is executed
     */
    virtual void handle(StreamResult res) = 0;
  };

}  // namespace libp2p::protocol

#endif  // KAGOME_BASE_PROTOCOL_HPP
