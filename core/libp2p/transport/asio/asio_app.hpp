/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_ASIO_APP_HPP
#define KAGOME_ASIO_APP_HPP

#define BOOST_ASIO_NO_DEPRECATED

#include <boost/asio.hpp>
#include <boost/thread.hpp>

namespace libp2p::transport::asio {

  /**
   * @brief Class, which owns boost::asio::io_context, thread pool. Should be 1
   * per application.
   */
  class AsioApp : private boost::asio::noncopyable {
   public:
    explicit AsioApp(int threads = 0);

    boost::asio::io_context &context();

    /**
     * @brief Run event loop.
     */
    void run();

    void run_for(std::chrono::milliseconds ms);

    ~AsioApp();
    AsioApp(const AsioApp &copy) = delete;
    AsioApp(AsioApp &&move) = delete;
    AsioApp &operator=(const AsioApp &copy) = delete;
    AsioApp &operator=(AsioApp &&move) = delete;

   private:
    int threads_;
    boost::asio::io_context context_;
    std::vector<std::thread> pool_;

    // this empty work prevents context from unblocking after run()
    using WorkType = boost::asio::executor_work_guard<
        boost::asio::io_context::executor_type>;
    std::unique_ptr<WorkType> work_;
  };

}  // namespace libp2p::transport::asio

#endif  // KAGOME_ASIO_APP_HPP
