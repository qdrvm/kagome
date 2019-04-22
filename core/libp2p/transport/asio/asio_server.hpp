/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_ASIO_SERVER_HPP
#define KAGOME_ASIO_SERVER_HPP

#define BOOST_ASIO_NO_DEPRECATED

#include "libp2p/basic/closeable.hpp"

#include <boost/asio.hpp>
#include <outcome/outcome.hpp>
#include "libp2p/multi/multiaddress.hpp"

namespace libp2p::transport::asio {

  /**
   * @brief Generic server.
   */
  class Server : public basic::Closeable {
   public:
    ~Server() override = default;

    /**
     * @brief Start accept clients.
     */
    virtual void startAccept() = 0;

    /**
     * @brief Get address server is listening on.
     * @throws boost::system::system_error if server is not valid (any valid
     * server is bound to an interface)
     * @return multiaddress
     */
    virtual multi::Multiaddress getMultiaddress() const = 0;
  };

}  // namespace libp2p::transport::asio

#endif  // KAGOME_ASIO_SERVER_HPP
