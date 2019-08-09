/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "core/api/extrinsic/simple_client.hpp"

namespace test {

  SimpleClient::SimpleClient(SimpleClient::Context &context,
                             SimpleClient::Duration timeout_duration,
                             SimpleClient::HandleTimeout on_timeout)
      : context_{context},
        socket_(context),
        deadline_timer_(context),
        timeout_duration_{timeout_duration},
        on_timeout_{std::move(on_timeout)} {}

  void SimpleClient::asyncConnect(const SimpleClient::Endpoint &endpoint,
                                  SimpleClient::HandleConnect on_success) {
    resetTimer();
    socket_.async_connect(
        endpoint, [handle = std::move(on_success)](const ErrorCode &error) {
          handle(error);
        });
  }

  void SimpleClient::asyncWrite(std::string_view data,
                                SimpleClient::HandleWrite on_success) {
    resetTimer();

    auto data_ptr = std::make_shared<std::string>(data);
    boost::asio::async_write(
        socket_,
        boost::asio::const_buffer(data_ptr->data(), data_ptr->size()),
        [data_ptr, handler = std::move(on_success)](
            ErrorCode error, std::size_t n) { handler(error, n); });
  }

  void SimpleClient::asyncRead(const SimpleClient::HandleRead &on_success) {
    resetTimer();
    boost::asio::async_read_until(
        socket_,
        buffer_,
        '\n',
        [this, on_success](ErrorCode error, std::size_t n) {
          if (!error) {
            read_count_ = n;
            on_success(error, n);
          }
        });
  }

  std::string SimpleClient::data() const {
    boost::asio::streambuf::const_buffers_type bufs = buffer_.data();
    return std::string(boost::asio::buffers_begin(bufs),
                       boost::asio::buffers_begin(bufs) + read_count_);
  }

  void SimpleClient::stop() {
    deadline_timer_.cancel();
    boost::system::error_code ignored_error;
    socket_.close(ignored_error);
  }

  void SimpleClient::resetTimer() {
    deadline_timer_.cancel();
    deadline_timer_.async_wait(boost::bind(&SimpleClient::onTimeout, this, _1));
    deadline_timer_.expires_after(timeout_duration_);
  }

  void SimpleClient::onTimeout(SimpleClient::ErrorCode) {
    if (deadline_timer_.expiry()
        <= boost::asio::steady_timer::clock_type::now()) {
      on_timeout_();
    }
  }

}  // namespace test
