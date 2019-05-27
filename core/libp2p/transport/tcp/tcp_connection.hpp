/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TCP_CONNECTION_HPP
#define KAGOME_TCP_CONNECTION_HPP

#define BOOST_ASIO_NO_DEPRECATED

#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>
#include <ufiber/ufiber.hpp>
#include "libp2p/connection/raw_connection.hpp"
#include "libp2p/multi/multiaddress.hpp"
#include "libp2p/transport/tcp/tcp_util.hpp"

namespace libp2p::transport {

  /**
   * @brief boost::asio implementation of TCP connection (socket).
   * @tparam Executor boost asio executor type. Can be a strand, io context or
   * custom.
   */
  template <typename Executor = boost::asio::io_context::executor_type>
  class TcpConnection
      : public connection::RawConnection,
        public std::enable_shared_from_this<TcpConnection<Executor>>,
        private boost::noncopyable {
   public:
    using Tcp = boost::asio::ip::tcp;
    using ResolverResultsType = Tcp::resolver::results_type;
    using executor_t = Executor;
    using yield_t = ufiber::yield_token<Executor>;

    TcpConnection(yield_t &yield, Tcp::socket &&socket)
        : yield_(yield), socket_(std::move(socket)) {}

    explicit TcpConnection(yield_t &yield)
        : yield_(yield), socket_(yield.get_executor().context()) {}

    outcome::result<TcpConnection::ResolverResultsType> resolve(
        const Tcp::endpoint &e) {
      Tcp::resolver resolver{yield_.get_executor().context()};
      auto [ec, iterator] = resolver.async_resolve(e, yield_);
      if (ec) {
        return handle_errcode(ec);
      }

      return iterator;
    }

    outcome::result<void> connect(const Tcp::endpoint &endpoint) {
      OUTCOME_TRY(resolved, resolve(endpoint));
      initiator_ = true;
      auto [ec, e] = boost::asio::async_connect(socket_, resolved, yield_);
      if (ec) {
        return handle_errcode(ec);
      }
      return outcome::success();
    }

    outcome::result<void> close() override {
      boost::system::error_code ec;
      socket_.close(ec);
      if (ec) {
        return handle_errcode(ec);
      }

      return outcome::success();
    }

    bool isClosed() const override {
      return !socket_.is_open();
    }

    outcome::result<multi::Multiaddress> remoteMultiaddr() override {
      return detail::makeEndpoint(socket_.remote_endpoint());
    }

    outcome::result<multi::Multiaddress> localMultiaddr() override {
      return detail::makeEndpoint(socket_.local_endpoint());
    }

    outcome::result<std::vector<uint8_t>> read(size_t bytes) override {
      std::vector<uint8_t> out(bytes, 0);
      OUTCOME_TRY(read(out));
      return out;
    };

    outcome::result<std::vector<uint8_t>> readSome(size_t bytes) override {
      std::vector<uint8_t> out(bytes, 0);
      OUTCOME_TRY(size, readSome(out));
      out.resize(size);
      return out;
    };

    outcome::result<size_t> read(gsl::span<uint8_t> buf) override {
      auto [ec, read] =
          boost::asio::async_read(socket_, detail::makeBuffer(buf), yield_);
      if (ec) {
        return handle_errcode(ec);
      }

      return read;
    };

    outcome::result<size_t> readSome(gsl::span<uint8_t> buf) override {
      auto [ec, read] =
          socket_.async_read_some(detail::makeBuffer(buf), yield_);
      if (ec) {
        return handle_errcode(ec);
      }

      return read;
    };

    outcome::result<size_t> write(gsl::span<const uint8_t> in) override {
      auto [ec, written] =
          boost::asio::async_write(socket_, detail::makeBuffer(in), yield_);
      if (ec) {
        return handle_errcode(ec);
      }

      return written;
    };

    outcome::result<size_t> writeSome(gsl::span<const uint8_t> in) override {
      auto [ec, written] =
          socket_.async_write_some(detail::makeBuffer(in), yield_);
      if (ec) {
        return handle_errcode(ec);
      }

      return written;
    };

    bool isInitiator() const noexcept override {
      return initiator_;
    }

   private:
    yield_t &yield_;
    Tcp::socket socket_;
    bool initiator_ = false;

    boost::system::error_code handle_errcode(
        const boost::system::error_code &e) noexcept {
      // TODO(warchant): handle client disconnected; handle connection timeout
      ////      if (e.category() == boost::asio::error::get_misc_category()) {
      //        if (e.value() == boost::asio::error::eof) {
      //          using connection::emits::OnConnectionAborted;
      //          emit<OnConnectionAborted>(OnConnectionAborted{});
      //        }
      ////      }

      return e;
    }
  };
}  // namespace libp2p::transport

#endif  // KAGOME_TCP_CONNECTION_HPP
