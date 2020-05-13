/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_APPLICATION_IMPL_BLOCK_PRODUCING_NODE_APPLICATION_HPP
#define KAGOME_CORE_APPLICATION_IMPL_BLOCK_PRODUCING_NODE_APPLICATION_HPP

#include "application/kagome_application.hpp"
#include "injector/block_producing_node_injector.hpp"

namespace kagome::application {

  class BlockProducingNodeApplication : public KagomeApplication {
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
    using Timer = clock::Timer;
    using InjectorType =
        decltype(injector::makeBlockProducingNodeInjector(std::string{},
                                                          std::string{},
                                                          std::string{},
                                                          uint16_t{},
                                                          uint16_t{},
                                                          uint16_t{}));

    template <class T>
    using sptr = std::shared_ptr<T>;

    template <class T>
    using uptr = std::unique_ptr<T>;

   public:
    ~BlockProducingNodeApplication() override = default;

    /**
     * @param kagome_config kagome configuration parameters
     * @param keys_config keys parameters
     */
    BlockProducingNodeApplication(const std::string &config_path,
                                  const std::string &keystore_path,
                                  const std::string &leveldb_path,
                                  uint16_t p2p_port,
                                  uint16_t rpc_http_port,
                                  uint16_t rpc_ws_port,
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
    sptr<api::ApiService> jrpc_api_service_;
    sptr<Babe> babe_;
    sptr<network::Router> router_;

    bool is_genesis_epoch_;
    common::Logger logger_;
  };

}  // namespace kagome::application

#endif  // KAGOME_CORE_APPLICATION_IMPL_BLOCK_PRODUCING_NODE_APPLICATION_HPP
