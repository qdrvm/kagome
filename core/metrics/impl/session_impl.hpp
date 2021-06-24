/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_METRICS_IMPL_SESSION_IMPL_HPP
#define KAGOME_CORE_METRICS_IMPL_SESSION_IMPL_HPP

#include <string_view>

#include <boost/asio/strand.hpp>
#include <boost/beast.hpp>
#include "log/logger.hpp"
#include "metrics/session.hpp"

namespace kagome::metrics {

  class SessionImpl : public Session,
                      public std::enable_shared_from_this<SessionImpl> {
    using Body = boost::beast::http::string_body;
    using Request = boost::beast::http::request<Body>;
    using Response = boost::beast::http::response<Body>;
    using Parser = boost::beast::http::request_parser<Body>;
    using HttpField = boost::beast::http::field;
    using HttpError = boost::beast::http::error;

   public:
    /**
     * @brief constructor
     * @param socket socket instance
     * @param config session configuration
     */
    SessionImpl(Context &context, Configuration config);
    ~SessionImpl() override = default;

    Socket &socket() override {
      return stream_.socket();
    }

    /**
     * @brief starts session
     */
    void start() override;

    /**
     * @brief sends response wrapped by http message
     * @param response message to send
     */
    void respond(Response response) override;

    /**
     * @brief method to get id of the session
     * @return id of the session
     */
    SessionId id() const override {
      return 0ull;
    }

   private:
    /**
     * @brief stops session
     */
    void stop();

    /**
     * @brief process http request, compose and execute response
     * @tparam Body request body type
     * @param request request
     */
    void handleRequest(Request &&request);

    /**
     * @brief asynchronously read http message
     */
    void asyncRead();

    /**
     * @brief sends http message
     * @tparam Message http message type
     * @param message http message
     */
    void asyncWrite(Response message);

    /**
     * @brief read completion callback
     */
    void onRead(boost::system::error_code ec, std::size_t size);

    /**
     * @brief write completion callback
     */
    void onWrite(bool close, boost::system::error_code ec, std::size_t);

    /**
     * @brief reports error code and message
     * @param ec error code
     * @param message error message
     */
    void reportError(boost::system::error_code ec, std::string_view message);

    static constexpr boost::string_view kServerName = "Kagome";

    /// Strand to ensure the connection's handlers are not called concurrently.
    boost::asio::strand<boost::asio::io_context::executor_type> strand_;

    Configuration config_;              ///< session configuration
    boost::beast::tcp_stream stream_;   ///< stream
    boost::beast::flat_buffer buffer_;  ///< read buffer
    std::shared_ptr<void> res_;

    /**
     * @brief request parser type
     */

    std::unique_ptr<Parser> parser_;  ///< http parser
    log::Logger logger_;
  };

}  // namespace kagome::metrics

#endif  // KAGOME_CORE_METRICS_IMPL_SESSION_IMPL_HPP
