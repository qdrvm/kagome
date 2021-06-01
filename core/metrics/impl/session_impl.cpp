/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "metrics/impl/session_impl.hpp"

#include <boost/system/error_code.hpp>

namespace kagome::metrics {

  SessionImpl::SessionImpl(Context &context, Configuration config)
      : strand_(boost::asio::make_strand(context)),
        config_{config},
        stream_(boost::asio::ip::tcp::socket(strand_)),
        logger_{log::createLogger("OpenMetricsSession", "metrics")} {}

  void SessionImpl::start() {
    asyncRead();
  }

  void SessionImpl::stop() {
    boost::system::error_code ec;
    stream_.socket().shutdown(Socket::shutdown_both, ec);
    boost::ignore_unused(ec);
  }

  void SessionImpl::handleRequest(Request &&req) {
    processRequest(req, shared_from_this());
  }

  void SessionImpl::asyncRead() {
    parser_ = std::make_unique<Parser>();
    parser_->body_limit(config_.max_request_size);
    stream_.expires_after(config_.operation_timeout);

    boost::beast::http::async_read(
        stream_,
        buffer_,
        parser_->get(),
        boost::beast::bind_front_handler(&SessionImpl::onRead,
                                         shared_from_this()));
  }

  void SessionImpl::asyncWrite(Response message) {
    auto m = std::make_shared<Response>(std::forward<Response>(message));

    res_ = m;

    // write response
    boost::beast::http::async_write(
        stream_,
        *m,
        boost::beast::bind_front_handler(
            &SessionImpl::onWrite, shared_from_this(), m->need_eof()));
  }

  void SessionImpl::respond(Response response) {
    return asyncWrite(response);
  }

  void SessionImpl::onRead(boost::system::error_code ec, std::size_t) {
    if (ec) {
      if (HttpError::end_of_stream != ec) {
        reportError(ec, "unknown error occurred");
      }

      stop();
    }

    handleRequest(parser_->release());
  }

  void SessionImpl::onWrite(bool close,
                            boost::system::error_code ec,
                            std::size_t) {
    if (ec) {
      reportError(ec, "failed to write message");
      return stop();
    }

    if (close) {
      return stop();
    }

    res_ = nullptr;

    // read next request
    asyncRead();
  }

  void SessionImpl::reportError(boost::system::error_code ec,
                                std::string_view message) {
    logger_->error("error occured: {}, code: {}, message: {}",
                   message,
                   ec.value(),
                   ec.message());
  }

}  // namespace kagome::metrics
