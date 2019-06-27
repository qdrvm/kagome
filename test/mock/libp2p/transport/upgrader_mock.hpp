/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_UPGRADER_MOCK_HPP
#define KAGOME_UPGRADER_MOCK_HPP

#include "libp2p/transport/upgrader.hpp"

#include <gmock/gmock.h>
#include "libp2p/muxer/yamux.hpp"
#include "libp2p/security/plaintext.hpp"

namespace libp2p::transport {

  class UpgraderMock : public Upgrader {
   public:
    ~UpgraderMock() override = default;

    MOCK_METHOD2(upgradeToSecure,
                 void(Upgrader::RawSPtr, Upgrader::OnSecuredCallbackFunc));

    MOCK_METHOD2(upgradeToMuxed,
                 void(Upgrader::SecSPtr, Upgrader::OnMuxedCallbackFunc));
  };

  /**
   * Upgrader, which upgrades raw connection to plaintexted (inbound) and
   * secured to Yamuxed
   */
  class DefaultUpgrader : public Upgrader {
    std::shared_ptr<security::SecurityAdaptor> security_adaptor_ =
        std::make_shared<security::Plaintext>();
    std::shared_ptr<muxer::MuxerAdaptor> muxer_adaptor_ =
        std::make_shared<muxer::Yamux>();

   public:
    void upgradeToSecure(RawSPtr conn, OnSecuredCallbackFunc cb) override {
      security_adaptor_->secureInbound(std::move(conn), std::move(cb));
    }

    void upgradeToMuxed(SecSPtr conn, OnMuxedCallbackFunc cb) override {
      muxer_adaptor_->muxConnection(std::move(conn), std::move(cb));
    }
  };

}  // namespace libp2p::transport

#endif  // KAGOME_UPGRADER_MOCK_HPP
