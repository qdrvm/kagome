/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_APPLICATION_IMPL_KAGOME_APPLICATION_IMPL_HPP
#define KAGOME_CORE_APPLICATION_IMPL_KAGOME_APPLICATION_IMPL_HPP

#include "application/configuration_storage.hpp"
#include "application/impl/local_key_storage.hpp"
#include "application/kagome_application.hpp"

namespace kagome::application {

  /**
   * @class KagomeApplicationImpl implements kagome application
   */
  class KagomeApplicationImpl : public KagomeApplication {
    using AuthorityIndex = primitives::AuthorityIndex;
    using Babe = consensus::Babe;
    using BabeGossiper = network::BabeGossiper;
    using BabeLottery = consensus::BabeLottery;
    using BlockBuilderFactory = authorship::BlockBuilderFactory;
    using BlockTree = blockchain::BlockTree;
    using Epoch = consensus::Epoch;
    using ExtrinsicApiService = api::ExtrinsicApiService;
    using Hasher = crypto::Hasher;
    using ListenerImpl = api::ListenerImpl;
    using Proposer = authorship::Proposer;
    using SR25519Keypair = crypto::SR25519Keypair;
    using Synchronizer = consensus::Synchronizer;
    using SystemClock = clock::SystemClock;
    using Timer = clock::Timer;

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
    KagomeApplicationImpl(KagomeConfig kagome_config,
                          LocalKeyStorage::Config keys_config);

    void run() override;

   private:
    Epoch makeInitialEpoch();

    // need to keep all of these instances, since injector itself is destroyed
    sptr<boost::asio::io_context> io_context_;
    sptr<ConfigurationStorage> config_storage_;
    sptr<KeyStorage> key_storage_;
    sptr<clock::SystemClock> clock_;
    sptr<ExtrinsicApiService> extrinsic_api_service_;
    sptr<Babe> babe_;
  };

}  // namespace kagome::application

#endif  // KAGOME_CORE_APPLICATION_IMPL_KAGOME_APPLICATION_IMPL_HPP
