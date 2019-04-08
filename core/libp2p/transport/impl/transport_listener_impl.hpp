/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TRANSPORT_LISTENER_IMPL_HPP
#define KAGOME_TRANSPORT_LISTENER_IMPL_HPP

#define BOOST_ASIO_NO_DEPRECATED

#include <list>

#include <boost/asio.hpp>
#include <boost/signals2.hpp>
#include "libp2p/multi/multiaddress.hpp"
#include "libp2p/transport/asio/asio_server.hpp"
#include "libp2p/transport/asio/asio_server_factory.hpp"
#include "libp2p/transport/transport_listener.hpp"

namespace libp2p::transport {

  class TransportListenerImpl : public TransportListener,
                                public asio::ServerFactory {
    using NoArgsSignal = boost::signals2::signal<NoArgsCallback>;
    using MultiaddrSignal = boost::signals2::signal<MultiaddrCallback>;
    using ErrorSignal = boost::signals2::signal<ErrorCallback>;
    using ConnectionSignal = boost::signals2::signal<ConnectionCallback>;

   public:
    ~TransportListenerImpl() override = default;

    TransportListenerImpl(boost::asio::io_context &context,
                                   HandlerFunc handler);

    outcome::result<void> listen(const multi::Multiaddress &address) override;

    /**
     * Get addresses, which this listener listens to
     * @return collection of those addresses
     */
    std::vector<multi::Multiaddress> getAddresses() const override;

    server_ptr_result ipTcp(const Address &ip, uint16_t port) const override;

    bool isClosed() const override;

    outcome::result<void> close() override;

    outcome::result<void> close(const multi::Multiaddress &ma) override;

    boost::signals2::connection onStartListening(
        std::function<MultiaddrCallback> callback) override;

    boost::signals2::connection onNewConnection(
        std::function<ConnectionCallback> callback) override;

    boost::signals2::connection onError(
        std::function<ErrorCallback> callback) override;

    boost::signals2::connection onClose(
        std::function<MultiaddrCallback> callback) override;

   private:
    boost::asio::io_context &context_;

    const HandlerFunc handler_;

    std::list<server_ptr> servers_;

    MultiaddrSignal signal_start_listening_{};
    ConnectionSignal signal_new_connection_{};
    ErrorSignal signal_error_{};
    MultiaddrSignal signal_close_{};
  };

}  // namespace libp2p::transport

#endif  // KAGOME_TRANSPORT_LISTENER_IMPL_HPP
