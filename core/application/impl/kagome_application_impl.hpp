/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_APPLICATION_IMPL_KAGOME_APPLICATION_IMPL_HPP
#define KAGOME_CORE_APPLICATION_IMPL_KAGOME_APPLICATION_IMPL_HPP

#include "application/kagome_application.hpp"

#include "api/service/api_service.hpp"
#include "application/configuration_storage.hpp"
#include "application/impl/local_key_storage.hpp"
#include "injector/full_node_injector.hpp"

namespace kagome::application {

  /**
   * @class KagomeApplicationImpl implements kagome application
   */
  class KagomeApplicationImpl : public KagomeApplication {
    using AuthorityIndex = primitives::AuthorityIndex;
    using Babe = consensus::Babe;
    using BabeGossiper = network::Gossiper;
    using BabeLottery = consensus::BabeLottery;
    using BlockBuilderFactory = authorship::BlockBuilderFactory;
    using BlockTree = blockchain::BlockTree;
    using Epoch = consensus::Epoch;
    using Hasher = crypto::Hasher;
    using ListenerImpl = api::WsListenerImpl;
    using Proposer = authorship::Proposer;
    using SR25519Keypair = crypto::SR25519Keypair;
    using Synchronizer = consensus::Synchronizer;
    using SystemClock = clock::SystemClock;
    using GrandpaLauncher = consensus::grandpa::Launcher;
    using Timer = clock::Timer;
    using InjectorType = decltype(
        injector::makeFullNodeInjector(std::string{},
                                       std::string{},
                                       std::string{},
                                       uint16_t{},
                                       boost::asio::ip::tcp::endpoint{},
                                       boost::asio::ip::tcp::endpoint{}));

    template <class T>
    using sptr = std::shared_ptr<T>;

    template <class T>
    using uptr = std::unique_ptr<T>;

   public:
    ~KagomeApplicationImpl() override = default;

    /**
     * @param kagome_config kagome configuration parameters
     * @param keys_config keys parameters
     */
    KagomeApplicationImpl(
        const std::string &config_path,
        const std::string &keystore_path,
        const std::string &leveldb_path,
        uint16_t p2p_port,
        const boost::asio::ip::tcp::endpoint &rpc_http_endpoint,
        const boost::asio::ip::tcp::endpoint &rpc_ws_endpoint,
        bool is_genesis_epoch,
        uint8_t verbosity);

    void run() override;

   private:
    // need to keep all of these instances, since injector itself is destroyed
    InjectorType injector_;
    sptr<boost::asio::io_context> io_context_;
    sptr<ConfigurationStorage> config_storage_;
    sptr<KeyStorage> key_storage_;
    sptr<clock::SystemClock> clock_;
    sptr<Babe> babe_;
    sptr<GrandpaLauncher> grandpa_launcher_;
    sptr<network::Router> router_;

    sptr<api::RpcContext> rpc_context_;
    sptr<api::RpcThreadPool> rpc_thread_pool_;
    sptr<api::ApiService> jrpc_api_service_;

    bool is_genesis_epoch_;
    common::Logger logger_;
  };

}  // namespace kagome::application

#endif  // KAGOME_CORE_APPLICATION_IMPL_KAGOME_APPLICATION_IMPL_HPP
