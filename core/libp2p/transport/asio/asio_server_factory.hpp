/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_ASIO_SERVER_FACTORY_HPP
#define KAGOME_ASIO_SERVER_FACTORY_HPP

#define BOOST_ASIO_NO_DEPRECATED

#include <memory>

#include <boost/asio.hpp>

#include "libp2p/transport/asio/asio_server.hpp"


namespace libp2p::transport::asio {

  /**
   * @brief Abstract factory to create specific servers.
   */
  class ServerFactory {
   public:
    virtual ~ServerFactory() = default;

    using Address = boost::asio::ip::address;
    using server_ptr = std::unique_ptr<asio::Server>;
    using server_ptr_result = outcome::result<server_ptr>;

    /**
     * @brief creates ip4/ip6 tcp client
     * @param ip ip4 or ip6
     * @param port tcp port
     * @return result on the pointer to specific server.
     */
    virtual server_ptr_result ipTcp(const Address &ip, uint16_t port) const = 0;
  };

}  // namespace libp2p::transport::asio

#endif  // KAGOME_ASIO_SERVER_FACTORY_HPP
