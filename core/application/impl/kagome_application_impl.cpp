/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/impl/kagome_application_impl.hpp"

namespace kagome::application {
  using consensus::Epoch;
  using std::chrono_literals::operator""ms;
  using consensus::Randomness;
  using consensus::Threshold;

  KagomeApplicationImpl::KagomeApplicationImpl(
      KagomeConfig kagome_config, LocalKeyStorage::Config keys_config) {
    auto &&injector = injector::makeApplicationInjector(
        injector::useConfig(kagome_config), injector::useConfig(keys_config));

    // keep important instances, the must exist when injector destroyed
    // some of them are requested by reference and hence not copied
    io_context_ = injector.create<sptr<boost::asio::io_context>>();
    config_storage_ = injector.create<sptr<ConfigurationStorage>>();
    key_storage_ = injector.create<sptr<KeyStorage>>();
    clock_ = injector.create<sptr<clock::SystemClock>>();
    extrinsic_api_service_ = injector.create<sptr<ExtrinsicApiService>>();
    babe_ = injector.create<uptr<Babe>>();
  }

  // TODO (yuraz) rewrite when there will be mor info
  Epoch KagomeApplicationImpl::makeInitialEpoch() {
    std::vector<primitives::Authority> authorities;
    auto &&session_keys = config_storage_->getSessionKeys();
    authorities.reserve(session_keys.size());

    for (auto &item : session_keys) {
      authorities.push_back(primitives::Authority{{item}, 1u});
    }

    Threshold threshold =
        boost::multiprecision::uint256_t(0.58 * 1e77) / authorities.size();

    Randomness rnd{};
    rnd.fill(0u);

    Epoch value{.epoch_index = 0u,
                .start_slot = 0u,
                .epoch_duration = 10,
                .slot_duration = 1000ms,
                .authorities = authorities,
                .threshold = threshold,
                .randomness = rnd};

    return value;
  }

  void KagomeApplicationImpl::run() {
    extrinsic_api_service_->start();
    auto epoch = makeInitialEpoch();
    babe_->runEpoch(std::move(epoch), clock_->now());
    io_context_->run();
  }
}  // namespace kagome::application
