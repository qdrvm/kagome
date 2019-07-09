/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_CORE_API_EXTRINSIC_SIMPLE_CLIENT_HPP
#define KAGOME_TEST_CORE_API_EXTRINSIC_SIMPLE_CLIENT_HPP

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/signals2/signal.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/write.hpp>


namespace test {

  /**
   * @class simple client for testing api service
   */
  class SimpleClient {
    template <class T>
    using Signal = boost::signals2::signal<T>;

    template <class T>
    using Function = boost::function<T>;

   public:
    using Endpoint = boost::asio::ip::tcp::endpoint;
    using Context = boost::asio::io_context;
    using Socket = boost::asio::ip::tcp::socket;
    using Timer = boost::asio::steady_timer;
    using Streambuf = boost::asio::streambuf;
    using Duration = boost::asio::steady_timer::duration;
    using ErrorCode = boost::system::error_code;

    using HandleConnect = Function<void(ErrorCode error)>;
    using HandleWrite = Function<void(ErrorCode error, std::size_t n)>;
    using HandleRead = Function<void(ErrorCode error, std::size_t n)>;
    using HandleTimeout = Function<void()>;

    /**
     * @brief constructor
     * @param context boost::asio::io_context
     */
    SimpleClient(Context &context, Duration timeout_duration,
                 HandleTimeout on_timeout);

    void asyncConnect(const Endpoint &endpoint, HandleConnect on_success);

    void asyncWrite(std::string data, HandleWrite on_success);

    void asyncRead(HandleRead on_success);

    std::string data() const;

    void stop();

   private:
    void resetTimer();

    void onTimeout(ErrorCode error);

    Socket socket_;
    Timer deadline_timer_;
    Streambuf buffer_;
    Duration timeout_duration_;
    HandleTimeout on_timeout_;
    std::size_t read_count_{0};
  };
}  // namespace test

#endif  // KAGOME_TEST_CORE_API_EXTRINSIC_SIMPLE_CLIENT_HPP
