/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_ASIO_CLIENT_FACTORY_HPP
#define KAGOME_ASIO_CLIENT_FACTORY_HPP

#define BOOST_ASIO_NO_DEPRECATED

#include <memory>

#include <boost/asio.hpp>

#include "libp2p/transport/connection.hpp"

namespace libp2p::transport::asio {

  /**
   * @brief Abstract factory to create specific servers.
   */
  class ClientFactory {
   public:
    virtual ~ClientFactory() = default;

    using Address = boost::asio::ip::address;
    using client_ptr = std::shared_ptr<Connection>;
    using client_ptr_result = outcome::result<client_ptr>;

    virtual client_ptr_result ipTcp(const Address &ip, uint16_t port) const = 0;
  };

}  // namespace libp2p::transport::asio

#endif  // KAGOME_ASIO_CLIENT_FACTORY_HPP
