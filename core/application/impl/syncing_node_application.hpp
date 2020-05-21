/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_APPLICATION_IMPL_SYNCING_NODE_APPLICATION_HPP
#define KAGOME_CORE_APPLICATION_IMPL_SYNCING_NODE_APPLICATION_HPP

#include "application/kagome_application.hpp"

#include <boost/asio/signal_set.hpp>

#include "common/logger.hpp"
#include "injector/syncing_node_injector.hpp"

namespace kagome::application {

  class SyncingNodeApplication : public KagomeApplication {
    template <class T>
    using sptr = std::shared_ptr<T>;

    template <class T>
    using uptr = std::unique_ptr<T>;

   public:
    using InjectorType = decltype(injector::makeSyncingNodeInjector(
        std::string{}, std::string{}, uint16_t{}, uint16_t{}, uint16_t{}));

    ~SyncingNodeApplication() override = default;

    SyncingNodeApplication(const std::string &config_path,
                           const std::string &leveldb_path,
                           uint16_t p2p_port,
                           uint16_t rpc_http_port,
                           uint16_t rpc_ws_port,
                           uint8_t verbosity);

    void run() override;
    void shutdown() override;

   private:
    // need to keep all of these instances, since injector itself is destroyed
    InjectorType injector_;
    sptr<boost::asio::io_context> io_context_;
    std::unique_ptr<boost::asio::signal_set> signals_;
    sptr<ConfigurationStorage> config_storage_;
    sptr<api::ApiService> jrpc_api_service_;
    sptr<network::Router> router_;

    common::Logger logger_;
  };

}  // namespace kagome::application

#endif  // KAGOME_CORE_APPLICATION_IMPL_SYNCING_NODE_APPLICATION_HPP
