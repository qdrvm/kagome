/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_APPLICATION_IMPL_BLOCK_PRODUCING_NODE_APPLICATION_HPP
#define KAGOME_CORE_APPLICATION_IMPL_BLOCK_PRODUCING_NODE_APPLICATION_HPP

#include "application/kagome_application.hpp"

#include "application/app_config.hpp"
#include "injector/block_producing_node_injector.hpp"

namespace kagome::application {

  class BlockProducingNodeApplication : public KagomeApplication {
    using Babe = consensus::Babe;
    using InjectorType =
        decltype(injector::makeBlockProducingNodeInjector(AppConfigPtr{}));

    template <class T>
    using sptr = std::shared_ptr<T>;

    template <class T>
    using uptr = std::unique_ptr<T>;

   public:
    ~BlockProducingNodeApplication() override = default;

    /**
     * @param kagome_config kagome configuration parameters
     */
    BlockProducingNodeApplication(AppConfigPtr app_config);

    void run() override;

   private:
    // need to keep all of these instances, since injector itself is destroyed
    InjectorType injector_;

    std::shared_ptr<AppStateManager> app_state_manager_;

    sptr<boost::asio::io_context> io_context_;

    sptr<ConfigurationStorage> config_storage_;
    sptr<KeyStorage> key_storage_;
    sptr<clock::SystemClock> clock_;
    sptr<Babe> babe_;
    sptr<network::Router> router_;

    sptr<api::ApiService> jrpc_api_service_;

    common::Logger logger_;
  };

}  // namespace kagome::application

#endif  // KAGOME_CORE_APPLICATION_IMPL_BLOCK_PRODUCING_NODE_APPLICATION_HPP
