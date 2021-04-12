/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_APPLICATION_IMPL_VALIDATING_NODE_APPLICATION_HPP
#define KAGOME_CORE_APPLICATION_IMPL_VALIDATING_NODE_APPLICATION_HPP

#include "application/kagome_application.hpp"

#include "injector/application_injector.hpp"
#include "api/service/api_service.hpp"
#include "application/app_configuration.hpp"
#include "application/chain_spec.hpp"
#include "application/app_state_manager.hpp"
#include "consensus/babe/babe.hpp"
#include "consensus/grandpa/grandpa.hpp"
#include "network/peer_manager.hpp"
#include "network/router.hpp"

namespace kagome::application {

  class ValidatingNodeApplication final: public KagomeApplication {
    using Babe = consensus::babe::Babe;
    using Grandpa = consensus::grandpa::Grandpa;

    template <class T>
    using sptr = std::shared_ptr<T>;

    template <class T>
    using uptr = std::unique_ptr<T>;

   public:
    ~ValidatingNodeApplication() override = default;

    /**
     * @param config_path genesis configs path
     * @param keystore_path local peer's keys
     * @param leveldb_path storage path
     * @param p2p_port port for p2p interactions
     * @param rpc_http_endpoint endpoint for http based rpc
     * @param rpc_ws_endpoint endpoint for ws based rpc
     * @param is_only_finalizing true if this node should be the only finalizing
     * node
     * @param verbosity level of logging
     */
    explicit ValidatingNodeApplication(const AppConfiguration &config);

    void run() override;

   private:
    log::Logger logger_;

    uptr<injector::ValidatingNodeInjector> injector_;
    sptr<boost::asio::io_context> io_context_;
    sptr<AppStateManager> app_state_manager_;
    sptr<ChainSpec> chain_spec_;
    sptr<clock::SystemClock> clock_;
    sptr<Babe> babe_;
    sptr<Grandpa> grandpa_;
    sptr<network::Router> router_;
    sptr<network::PeerManager> peer_manager_;
    sptr<api::ApiService> jrpc_api_service_;

    Babe::ExecutionStrategy babe_execution_strategy_;

    boost::filesystem::path chain_path_;
    const std::string node_name_;
  };

}  // namespace kagome::application

#endif  // KAGOME_CORE_APPLICATION_IMPL_VALIDATING_NODE_APPLICATION_HPP
